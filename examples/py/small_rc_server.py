#!/usr/bin/env python3
"""
Small RC server using raw verbs (no RDMA CM QP creation).
Exchange QP info over TCP, then wait for a WRITE.
"""

import json
import random
import socket
import struct
import sys

from pyverbs.pd import PD
from pyverbs.cq import CQ
from pyverbs.qp import QP, QPCap, QPInitAttr, QPAttr
from pyverbs.addr import AHAttr, GlobalRoute
import pyverbs.device as pv_dev

from rdma_compat import create_mr, mr_read, mr_write, get_qp_type_rc

try:
    from pyverbs.qp import QPState
except Exception:
    QPState = None
try:
    from pyverbs.pyverbs_enums import (
        IBV_ACCESS_LOCAL_WRITE,
        IBV_ACCESS_REMOTE_READ,
        IBV_ACCESS_REMOTE_WRITE,
        IBV_MTU_1024,
        IBV_QPS_INIT,
        IBV_QPS_RTR,
        IBV_QPS_RTS,
    )
except Exception:
    from pyverbs.enums import (
        IBV_ACCESS_LOCAL_WRITE,
        IBV_ACCESS_REMOTE_READ,
        IBV_ACCESS_REMOTE_WRITE,
        IBV_MTU_1024,
        IBV_QPS_INIT,
        IBV_QPS_RTR,
        IBV_QPS_RTS,
    )


BUF_SZ = 4096


def _qp_state(name):
    if QPState is not None:
        return getattr(QPState, name)
    return {
        "INIT": IBV_QPS_INIT,
        "RTR": IBV_QPS_RTR,
        "RTS": IBV_QPS_RTS,
    }[name]


def _list_devices():
    if hasattr(pv_dev, "DeviceList"):
        return pv_dev.DeviceList()
    if hasattr(pv_dev, "get_device_list"):
        return pv_dev.get_device_list()
    return []


def _device_name(dev):
    if hasattr(dev, "name"):
        val = dev.name
        val = val() if callable(val) else val
        if isinstance(val, bytes):
            return val.decode("utf-8", "ignore")
        return val
    if hasattr(dev, "device_name"):
        val = dev.device_name
        val = val() if callable(val) else val
        if isinstance(val, bytes):
            return val.decode("utf-8", "ignore")
        return val
    return str(dev)


def _get_device(name):
    for dev in _list_devices():
        if _device_name(dev) == name:
            return dev
    return None


def _rand_psn():
    return random.getrandbits(24)


def _to_init(qp, port):
    attr = QPAttr()
    attr.qp_state = _qp_state("INIT")
    attr.port_num = port
    attr.pkey_index = 0
    attr.qp_access_flags = (
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
    )
    qp.to_init(attr)


def _to_rtr(qp, port, peer_qpn, peer_psn, peer_lid, peer_gid):
    attr = QPAttr()
    attr.qp_state = _qp_state("RTR")
    attr.path_mtu = IBV_MTU_1024
    attr.dest_qp_num = peer_qpn
    attr.rq_psn = peer_psn
    attr.max_dest_rd_atomic = 1
    attr.min_rnr_timer = 12

    ah = AHAttr()
    ah.dlid = peer_lid
    ah.sl = 0
    ah.src_path_bits = 0
    ah.port_num = port
    if peer_lid == 0 and peer_gid:
        if not hasattr(ah, "grh"):
            raise RuntimeError("AHAttr.grh not available; cannot set GRH for RoCE")
        grh = GlobalRoute()
        grh.dgid = peer_gid
        grh.sgid_index = 0
        grh.hop_limit = 1
        ah.is_global = 1
        ah.grh = grh
    attr.ah_attr = ah
    qp.to_rtr(attr)


def _to_rts(qp, psn):
    attr = QPAttr()
    attr.qp_state = _qp_state("RTS")
    attr.sq_psn = psn
    attr.timeout = 14
    attr.retry_cnt = 7
    attr.rnr_retry = 7
    attr.max_rd_atomic = 1
    qp.to_rts(attr)


def _send_msg(conn, obj):
    data = json.dumps(obj).encode("utf-8")
    conn.sendall(struct.pack("!I", len(data)))
    conn.sendall(data)


def _recv_msg(conn):
    raw = conn.recv(4)
    if len(raw) < 4:
        raise RuntimeError("short read")
    n = struct.unpack("!I", raw)[0]
    data = b""
    while len(data) < n:
        chunk = conn.recv(n - len(data))
        if not chunk:
            raise RuntimeError("short read")
        data += chunk
    return json.loads(data.decode("utf-8"))


def main():
    if len(sys.argv) < 2:
        print("Usage: small_rc_server.py <tcp_port> [ib_dev] [ib_port] [gid_index]")
        return 1

    tcp_port = int(sys.argv[1])
    ib_dev = sys.argv[2] if len(sys.argv) >= 3 else "rxe0"
    ib_port = int(sys.argv[3]) if len(sys.argv) >= 4 else 1
    gid_index = int(sys.argv[4]) if len(sys.argv) >= 5 else 0

    dev = _get_device(ib_dev)
    if dev is None:
        raise RuntimeError(f"Device '{ib_dev}' not found")
    try:
        ctx = pv_dev.Context(name=dev)
    except Exception:
        ctx = pv_dev.Context(name=_device_name(dev))
    pd = PD(ctx)
    cq = CQ(ctx, cqe=64)

    buf = bytearray(BUF_SZ)
    buf[0:13] = b"server-initial"
    mr = create_mr(pd, buf, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE)
    mr_write(mr, buf)

    cap = QPCap(max_send_wr=64, max_recv_wr=16, max_send_sge=1, max_recv_sge=1)
    init_attr = QPInitAttr(qp_type=get_qp_type_rc(), scq=cq, rcq=cq, cap=cap, sq_sig_all=True)
    qp = QP(pd, init_attr)

    port_attr = ctx.query_port(ib_port)
    lid = getattr(port_attr, "lid", 0)
    gid = str(ctx.query_gid(ib_port, gid_index))
    psn = _rand_psn()

    _to_init(qp, ib_port)

    info = {
        "qpn": getattr(qp, "qp_num", None),
        "psn": psn,
        "lid": lid,
        "gid": gid,
        "rkey": mr.rkey,
        "addr": mr.buf,
    }

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(("", tcp_port))
        s.listen(1)
        print(f"[rc-server] listening on TCP {tcp_port}")
        conn, _ = s.accept()
        with conn:
            _send_msg(conn, info)
            peer = _recv_msg(conn)

    _to_rtr(qp, ib_port, peer["qpn"], peer["psn"], peer["lid"], peer["gid"])
    _to_rts(qp, psn)

    print("[rc-server] ready; waiting for client write...")
    time.sleep(1)
    text = mr_read(mr, BUF_SZ).split(b"\x00")[0].decode("utf-8", "ignore")
    print(f"[rc-server] buffer: '{text}'")
    return 0


if __name__ == "__main__":
    import time
    sys.exit(main())

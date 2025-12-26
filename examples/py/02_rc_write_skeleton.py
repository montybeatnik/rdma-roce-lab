#!/usr/bin/env python3
"""
RC QP setup + RDMA WRITE skeleton using pyverbs.

This is a teaching scaffold. It creates a context, PD, CQ, MR, and an RC QP.
If you provide peer details, it will attempt to transition QP states and issue
an RDMA WRITE. Without peer details, it prints what you need to exchange.
"""

import argparse
import os
import random
import sys

try:
    from pyverbs.device import Context, DeviceList
    from pyverbs.pd import PD
    from pyverbs.cq import CQ
    from pyverbs.mr import MR
    from pyverbs.qp import QP, QPCap, QPInitAttr, QPAttr, QPState, QPType
    from pyverbs.addr import AHAttr, GlobalRoute
    from pyverbs.pyverbs_enums import (
        IBV_ACCESS_LOCAL_WRITE,
        IBV_ACCESS_REMOTE_READ,
        IBV_ACCESS_REMOTE_WRITE,
        IBV_MTU_1024,
    )
    from pyverbs.wr import SGE, SendWR
except ImportError as exc:
    print("pyverbs is not installed. Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc


def _get_device(name: str):
    for dev in DeviceList():
        if dev.name == name:
            return dev
    return None


def _rand_psn() -> int:
    return random.getrandbits(24)


def _print_exchange_info(port_attr, qp, psn, mr, gid_value):
    lid = getattr(port_attr, "lid", None)
    qpn = getattr(qp, "qp_num", None)
    print("Local connection info (share with peer):")
    print(f"  qpn: {qpn}")
    print(f"  psn: {psn}")
    if lid is not None:
        print(f"  lid: {lid}")
    if gid_value:
        print(f"  gid: {gid_value}")
    print(f"  rkey: {mr.rkey}")
    print(f"  addr: 0x{mr.buf:016x}")


def _query_gid(ctx: Context, port: int, gid_index: int) -> str:
    try:
        gid = ctx.query_gid(port, gid_index)
        return str(gid)
    except Exception:
        return ""


def _to_init(qp: QP, port: int):
    attr = QPAttr()
    attr.qp_state = QPState.INIT
    attr.port_num = port
    attr.pkey_index = 0
    attr.qp_access_flags = (
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE
    )
    qp.to_init(attr)


def _to_rtr(qp: QP, port: int, peer_qpn: int, peer_psn: int, peer_lid: int, peer_gid: str):
    attr = QPAttr()
    attr.qp_state = QPState.RTR
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
    if peer_gid:
        grh = GlobalRoute()
        grh.dgid = peer_gid
        grh.sgid_index = 0
        grh.hop_limit = 1
        ah.is_global = 1
        ah.grh = grh
    attr.ah_attr = ah
    qp.to_rtr(attr)


def _to_rts(qp: QP, psn: int):
    attr = QPAttr()
    attr.qp_state = QPState.RTS
    attr.sq_psn = psn
    attr.timeout = 14
    attr.retry_cnt = 7
    attr.rnr_retry = 7
    attr.max_rd_atomic = 1
    qp.to_rts(attr)


def _post_write(qp: QP, mr: MR, length: int, remote_addr: int, remote_rkey: int):
    sge = SGE(addr=mr.buf, length=length, lkey=mr.lkey)
    wr = SendWR(
        opcode=SendWR.RDMA_WRITE,
        num_sge=1,
        sg=[sge],
        send_flags=SendWR.SIGNALED,
        rkey=remote_rkey,
        remote_addr=remote_addr,
    )
    qp.post_send(wr)


def main() -> int:
    parser = argparse.ArgumentParser(description="RC QP setup + RDMA WRITE skeleton")
    parser.add_argument("device", help="RDMA device name (e.g., rxe0)")
    parser.add_argument("port", type=int, help="Port number (usually 1)")
    parser.add_argument("--gid-index", type=int, default=0, help="GID index")
    parser.add_argument("--buf-size", type=int, default=4096, help="Local MR size")
    parser.add_argument("--peer-qpn", type=int, default=None)
    parser.add_argument("--peer-psn", type=int, default=None)
    parser.add_argument("--peer-lid", type=int, default=0)
    parser.add_argument("--peer-gid", default="")
    parser.add_argument("--peer-rkey", type=lambda s: int(s, 0), default=None)
    parser.add_argument("--peer-addr", type=lambda s: int(s, 0), default=None)
    args = parser.parse_args()

    dev = _get_device(args.device)
    if dev is None:
        print(f"Device '{args.device}' not found.")
        return 1

    ctx = Context(dev)
    pd = PD(ctx)
    cq = CQ(ctx, cqe=128)

    buf = bytearray(args.buf_size)
    for i in range(len(buf)):
        buf[i] = 0x5A
    mr = MR(
        pd,
        buf,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE,
    )

    cap = QPCap(max_send_wr=64, max_recv_wr=16, max_send_sge=1, max_recv_sge=1)
    init_attr = QPInitAttr(
        qp_type=QPType.RC,
        scq=cq,
        rcq=cq,
        cap=cap,
        sq_sig_all=True,
    )
    qp = QP(pd, init_attr)

    local_psn = _rand_psn()
    port_attr = ctx.query_port(args.port)
    gid_value = _query_gid(ctx, args.port, args.gid_index)

    _print_exchange_info(port_attr, qp, local_psn, mr, gid_value)

    if (
        args.peer_qpn is None
        or args.peer_psn is None
        or args.peer_rkey is None
        or args.peer_addr is None
    ):
        print("\nPeer details missing; skipping QP state transitions and WRITE.")
        print("Provide --peer-qpn/--peer-psn/--peer-rkey/--peer-addr to attempt a WRITE.")
        return 0

    if os.geteuid() != 0:
        print("\nNote: RDMA ops may require elevated privileges or memlock limits.")

    _to_init(qp, args.port)
    _to_rtr(qp, args.port, args.peer_qpn, args.peer_psn, args.peer_lid, args.peer_gid)
    _to_rts(qp, local_psn)

    _post_write(qp, mr, min(args.buf_size, 4096), args.peer_addr, args.peer_rkey)
    print("RDMA WRITE posted. Poll the CQ to confirm completion.")
    return 0


if __name__ == "__main__":
    sys.exit(main())

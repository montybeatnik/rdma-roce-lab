#!/usr/bin/env python3
"""
Small RC CM client that talks to 10_minimal_server.py.
"""

import struct
import sys

from pyverbs.cm_enums import RDMA_PS_TCP, RDMA_CM_EVENT_ESTABLISHED
from pyverbs.cmid import CMEventChannel, ConnParam
from pyverbs.cq import CQ
from pyverbs.pd import PD
from pyverbs.qp import QPCap, QPInitAttr
from pyverbs.wr import SGE, SendWR

from rdma_compat import (
    build_qp_init_attr,
    create_cmid,
    create_mr,
    create_qp_compat,
    ensure_qp_init_attr,
    get_cmid_context,
    get_cmid_pd,
    get_conn_param_compat,
    get_qp_type_rc,
    mr_read,
    mr_write,
    resolve_addr_compat,
    resolve_route_compat,
)


BUF_SZ = 4096
INFO_FMT = "!QIQ"  # addr (u64), rkey (u32), len (u64)


def poll_one(cq):
    while True:
        if hasattr(cq, "poll"):
            res = cq.poll(1)
            if isinstance(res, tuple) and len(res) == 2:
                num, wcs = res
                if num > 0:
                    return wcs[0]
            elif isinstance(res, list) and len(res) > 0:
                return res[0]
        else:
            raise RuntimeError("CQ has no poll method")


def main():
    if len(sys.argv) < 2:
        print("Usage: small.py <server_ip> [port]")
        return 1

    server_ip = sys.argv[1]
    port = sys.argv[2] if len(sys.argv) >= 3 else "7471"

    ec = CMEventChannel()
    cmid = create_cmid(ec, RDMA_PS_TCP)
    resolve_addr_compat(cmid, server_ip, port, 2000)
    resolve_route_compat(cmid, 2000)

    ctx = get_cmid_context(cmid)
    if ctx is None:
        raise RuntimeError("CMID has no usable context")
    pd = get_cmid_pd(cmid) or PD(ctx)
    cq = CQ(ctx, cqe=32)

    tx_buf = bytearray(BUF_SZ)
    rx_buf = bytearray(BUF_SZ)
    tx_buf[0:18] = b"client-wrote-this"
    mr_tx = create_mr(pd, tx_buf, 0)
    mr_rx = create_mr(pd, rx_buf, 0)
    mr_write(mr_tx, tx_buf)
    mr_write(mr_rx, rx_buf)

    cap = QPCap(max_send_wr=32, max_recv_wr=32, max_send_sge=1, max_recv_sge=1)
    init_attr = build_qp_init_attr(QPInitAttr, get_qp_type_rc(), cq, cap, True)
    ensure_qp_init_attr(init_attr, cq)
    qp = create_qp_compat(cmid, pd, init_attr)
    if qp is None:
        raise RuntimeError("CMID did not expose a QP handle")

    conn_param = ConnParam(
        responder_resources=1,
        initiator_depth=1,
        retry_count=7,
        rnr_retry_count=7,
    )
    cmid.connect(conn_param)
    connp = get_conn_param_compat(ec, cmid, None, RDMA_CM_EVENT_ESTABLISHED)

    info = struct.unpack(INFO_FMT, connp.private_data[: struct.calcsize(INFO_FMT)])
    remote_addr, remote_rkey, _remote_len = info

    sge = SGE(addr=mr_tx.buf, length=len("client-wrote-this") + 1, lkey=mr_tx.lkey)
    wr = SendWR(
        opcode=SendWR.RDMA_WRITE,
        num_sge=1,
        sg=[sge],
        send_flags=SendWR.SIGNALED,
        rkey=remote_rkey,
        remote_addr=remote_addr,
    )
    qp.post_send(wr)
    poll_one(cq)

    sge = SGE(addr=mr_rx.buf, length=BUF_SZ, lkey=mr_rx.lkey)
    wr = SendWR(
        opcode=SendWR.RDMA_READ,
        num_sge=1,
        sg=[sge],
        send_flags=SendWR.SIGNALED,
        rkey=remote_rkey,
        remote_addr=remote_addr,
    )
    qp.post_send(wr)
    poll_one(cq)

    read_back = mr_read(mr_rx, BUF_SZ).split(b"\x00")[0].decode("utf-8", "ignore")
    print(f"[small] read back: '{read_back}'")
    return 0


if __name__ == "__main__":
    sys.exit(main())

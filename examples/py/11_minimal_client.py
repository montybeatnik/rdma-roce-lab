#!/usr/bin/env python3
"""
Minimal RDMA client (Python) using pyverbs + RDMA CM.

This mirrors the C minimal client: RDMA CM for connection + private_data for
the remote buffer info. It is intentionally explicit and low-magic.
"""

import struct
import sys
import os

try:
    from pyverbs.cmid import CMEventChannel, ConnParam
    from pyverbs.cm_enums import RDMA_PS_TCP, RDMA_CM_EVENT_ESTABLISHED
    from pyverbs.pd import PD
    from pyverbs.cq import CQ
    from pyverbs.qp import QPCap, QPInitAttr
    try:
        from pyverbs.pyverbs_enums import (
            IBV_ACCESS_LOCAL_WRITE,
            IBV_ACCESS_REMOTE_READ,
            IBV_ACCESS_REMOTE_WRITE,
        )
    except Exception:
        from pyverbs.enums import (
            IBV_ACCESS_LOCAL_WRITE,
            IBV_ACCESS_REMOTE_READ,
            IBV_ACCESS_REMOTE_WRITE,
        )
    from pyverbs.wr import SGE, SendWR
except Exception as exc:
    print("pyverbs import failed.")
    print(f"Error: {exc}")
    print("Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc

from rdma_compat import (
    build_qp_init_attr,
    create_cmid,
    create_mr,
    create_qp_compat,
    ensure_qp_init_attr,
    bind_addr_src_compat,
    get_cmid_context,
    get_cmid_pd,
    get_pd_context,
    get_conn_param_compat,
    get_qp_type_rc,
    mr_read,
    mr_write,
    resolve_addr_compat,
    resolve_route_compat,
)

QP_TYPE_RC = get_qp_type_rc()


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


def post_write(qp, mr, length, remote_addr, remote_rkey):
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


def post_read(qp, mr, length, remote_addr, remote_rkey):
    sge = SGE(addr=mr.buf, length=length, lkey=mr.lkey)
    wr = SendWR(
        opcode=SendWR.RDMA_READ,
        num_sge=1,
        sg=[sge],
        send_flags=SendWR.SIGNALED,
        rkey=remote_rkey,
        remote_addr=remote_addr,
    )
    qp.post_send(wr)


def main():
    if len(sys.argv) < 2:
        print("Usage: 11_minimal_client.py <server_ip> [port]")
        return 1

    server_ip = sys.argv[1]
    port = sys.argv[2] if len(sys.argv) >= 3 else "7471"

    # 1. Define Buffers FIRST to avoid NameError
    tx_buf = bytearray(BUF_SZ)
    rx_buf = bytearray(BUF_SZ)
    tx_buf[0:18] = b"client-wrote-this"

    # 2. Setup CMID and Resolve Route
    ec = CMEventChannel()
    cmid = create_cmid(ec, RDMA_PS_TCP)
    src_ip = os.environ.get("RDMA_SRC_IP")
    if src_ip:
        src_port = os.environ.get("RDMA_SRC_PORT")
        try:
            bind_addr_src_compat(cmid, src_ip, src_port)
        except Exception:
            pass
    resolve_addr_compat(cmid, server_ip, port, 2000)
    resolve_route_compat(cmid, 2000)

    # 3. Context/PD acquisition aligned with CMID
    pd = get_cmid_pd(cmid)
    ctx = get_pd_context(pd) if pd is not None else None
    if ctx is None:
        ctx = get_cmid_context(cmid)
    if ctx is None:
        raise RuntimeError("CMID has no usable context")
    if pd is None or get_pd_context(pd) is None:
        pd = PD(ctx)
    cq = CQ(ctx, cqe=64)
    if os.environ.get("RDMA_DEBUG") == "1":
        ctx_name = getattr(ctx, "name", None)
        if ctx_name is not None:
            ctx_name = ctx_name() if callable(ctx_name) else ctx_name
            if isinstance(ctx_name, bytes):
                ctx_name = ctx_name.decode("utf-8", "ignore")
        print(f"[client] Resources aligned on: {ctx_name}")

    # 4. Register Memory Regions
    mr_tx = create_mr(pd, tx_buf, IBV_ACCESS_LOCAL_WRITE)
    mr_rx = create_mr(pd, rx_buf, IBV_ACCESS_LOCAL_WRITE)
    mr_write(mr_tx, tx_buf)
    mr_write(mr_rx, rx_buf)

    # 5. Create QP (compat path)
    cap = QPCap(max_send_wr=32, max_recv_wr=32, max_send_sge=1, max_recv_sge=1)
    init_attr = build_qp_init_attr(QPInitAttr, QP_TYPE_RC, cq, cap, True)
    ensure_qp_init_attr(init_attr, cq)
    qp = create_qp_compat(cmid, pd, init_attr)
    if qp is None:
        raise RuntimeError("CMID did not expose a QP handle")

    # 6. Connect and Exchange Data
    conn_param = ConnParam(responder_resources=1, initiator_depth=1, retry_count=7, rnr_retry_count=7)
    cmid.connect(conn_param)
    
    # Wait for the connection event
    connp = get_conn_param_compat(ec, cmid, None, RDMA_CM_EVENT_ESTABLISHED)

    # 7. RDMA Operations
    info = struct.unpack(INFO_FMT, connp.private_data[: struct.calcsize(INFO_FMT)])
    remote_addr, remote_rkey, _ = info

    print(f"[client] Connected! Writing to {hex(remote_addr)}")
    post_write(qp, mr_tx, 19, remote_addr, remote_rkey)
    poll_one(cq)

    post_read(qp, mr_rx, BUF_SZ, remote_addr, remote_rkey)
    poll_one(cq)

    read_back = mr_read(mr_rx, BUF_SZ).split(b"\x00")[0].decode("utf-8", "ignore")
    print(f"[client] Result: '{read_back}'")

    return 0


if __name__ == "__main__":
    sys.exit(main())

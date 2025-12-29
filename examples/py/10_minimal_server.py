#!/usr/bin/env python3
"""
Minimal RDMA server (Python) using pyverbs + RDMA CM.

This mirrors the C minimal server: RDMA CM for connection + private_data for
the remote buffer info. It is intentionally explicit and low-magic.
"""

import struct
import sys
import time
import os

try:
    from pyverbs.cmid import CMEventChannel, ConnParam
    from pyverbs.cm_enums import RDMA_PS_TCP, RDMA_CM_EVENT_CONNECT_REQUEST, RDMA_CM_EVENT_ESTABLISHED
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
except Exception as exc:
    print("pyverbs import failed.")
    print(f"Error: {exc}")
    print("Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc

from rdma_compat import (
    bind_addr_compat,
    build_qp_init_attr,
    create_cmid,
    create_mr,
    create_qp_compat,
    ensure_qp_init_attr,
    get_cmid_context,
    get_cmid_pd,
    get_pd_context,
    get_qp_type_rc,
    get_request_compat,
    mr_read,
    mr_write,
    wait_established_compat,
)

QP_TYPE_RC = get_qp_type_rc()


BUF_SZ = 4096
INFO_FMT = "!QIQ"  # addr (u64), rkey (u32), len (u64)


def get_cmid_context(cmid):
    if hasattr(cmid, "ctx"):
        return cmid.ctx
    if hasattr(cmid, "context"):
        ctx_obj = cmid.context
        if callable(ctx_obj):
            ctx_val = ctx_obj()
            if ctx_val is not None:
                return ctx_val
        else:
            if ctx_obj is not None:
                return ctx_obj
    if hasattr(cmid, "pd"):
        pd_obj = cmid.pd() if callable(cmid.pd) else cmid.pd
        if pd_obj is not None and hasattr(pd_obj, "ctx"):
            ctx_obj = pd_obj.ctx
            if callable(ctx_obj):
                ctx_val = ctx_obj()
                if ctx_val is not None:
                    return ctx_val
            else:
                if ctx_obj is not None:
                    return ctx_obj
    return _fallback_context()


def main():
    if len(sys.argv) < 2:
        print("Usage: 10_minimal_server.py [port]")
        return 1

    port = sys.argv[1] if len(sys.argv) >= 2 else "7471"

    ec = CMEventChannel()
    if os.environ.get("RDMA_DEBUG") == "1":
        print("[server] created CMEventChannel")
    listen_id = create_cmid(ec, RDMA_PS_TCP)
    if not hasattr(listen_id, "bind_addr"):
        raise RuntimeError("CMID.bind_addr not available in this pyverbs build")
    bind_addr_compat(listen_id, port)
    listen_id.listen(1)
    if os.environ.get("RDMA_DEBUG") == "1":
        print("[server] listen() ok")

    print(f"[server] listening on RDMA CM port {port}")
    req = get_request_compat(listen_id, ec)
    if hasattr(req, "event"):
        if req.event != RDMA_CM_EVENT_CONNECT_REQUEST:
            raise RuntimeError(f"Unexpected CM event: {req.event}")
        cmid = req.id
        req.ack()
    else:
        cmid = req

    pd = get_cmid_pd(cmid)
    ctx = get_pd_context(pd) if pd is not None else None
    if ctx is None:
        ctx = get_cmid_context(cmid)
    if ctx is None:
        raise RuntimeError("CMID has no usable context")
    if pd is None or get_pd_context(pd) is None:
        pd = PD(ctx)
    if os.environ.get("RDMA_DEBUG") == "1":
        print(f"[server] ctx={ctx} pd={pd}")

    if os.environ.get("RDMA_CMID_OWNED_RESOURCES") == "1":
        os.environ["RDMA_QP_SKIP_PD"] = "1"
        os.environ["RDMA_QP_INIT_ONLY_FIRST"] = "1"
    cq = CQ(ctx, cqe=64)

    buf = bytearray(BUF_SZ)
    buf[0:14] = b"server-initial"
    mr = create_mr(
        pd,
        buf,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE,
    )
    mr_write(mr, buf)

    if not hasattr(cmid, "create_qp"):
        raise RuntimeError("CMID.create_qp not available in this pyverbs build")
    cap = QPCap(max_send_wr=32, max_recv_wr=32, max_send_sge=1, max_recv_sge=1)
    init_attr = build_qp_init_attr(QPInitAttr, QP_TYPE_RC, cq, cap, True)
    ensure_qp_init_attr(init_attr, cq)
    create_qp_compat(cmid, pd, init_attr)

    info = struct.pack(INFO_FMT, mr.buf, mr.rkey, BUF_SZ)
    conn_param = ConnParam(
        private_data=info,
        private_data_len=len(info),
        responder_resources=1,
        initiator_depth=1,
        retry_count=7,
        rnr_retry_count=7,
    )
    if not hasattr(cmid, "accept"):
        raise RuntimeError("CMID.accept not available in this pyverbs build")
    cmid.accept(conn_param)

    ev = wait_established_compat(cmid, ec)
    if ev is not None:
        if ev.event != RDMA_CM_EVENT_ESTABLISHED:
            raise RuntimeError(f"Unexpected CM event: {ev.event}")
        ev.ack()

    print("[server] connected; waiting for client ops...")

    time.sleep(1)
    nul = b"\x00"
    decoded = buf.split(nul)[0].decode("utf-8", "ignore")
    decoded = mr_read(mr, BUF_SZ).split(nul)[0].decode("utf-8", "ignore")
    print(f"[server] buffer after client ops: '{decoded}'")
    return 0


if __name__ == "__main__":
    sys.exit(main())

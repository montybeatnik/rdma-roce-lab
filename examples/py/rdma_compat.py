#!/usr/bin/env python3
"""
Compatibility helpers for pyverbs differences across builds.
"""

import ctypes
import os
import time
import sys


def get_qp_type_rc():
    try:
        from pyverbs.qp import QPType
        return QPType.RC
    except Exception:
        pass
    try:
        from pyverbs.pyverbs_enums import IBV_QPT_RC
        return IBV_QPT_RC
    except Exception:
        pass
    try:
        from pyverbs.enums import IBV_QPT_RC
        return IBV_QPT_RC
    except Exception:
        return 1


def _debug_enabled():
    return os.environ.get("RDMA_DEBUG") == "1"


def _debug(msg):
    if _debug_enabled():
        print(f"[rdma_compat] {msg}", file=sys.stderr)


def create_cmid(ec, port_space, cmid_cls=None):
    if cmid_cls is None:
        from pyverbs.cmid import CMID as cmid_cls
    try:
        cmid = cmid_cls(ec, port_space)
        _debug(f"create_cmid(ec, port_space={port_space}) -> {cmid}")
        return cmid
    except TypeError:
        try:
            cmid = cmid_cls(ec, None, port_space)
            _debug(f"create_cmid(ec, None, port_space={port_space}) -> {cmid}")
            return cmid
        except TypeError:
            cmid = cmid_cls(ec, qp_init_attr=None, port_space=port_space)
            _debug(f"create_cmid(ec, qp_init_attr=None, port_space={port_space}) -> {cmid}")
            return cmid


def bind_addr_compat(cmid, port, addrinfo_factory=None, port_space=None, passive_flag=None):
    try:
        cmid.bind_addr(None, port)
        _debug(f"bind_addr_compat(cmid, None, port={port}) ok")
        return
    except TypeError:
        pass
    try:
        cmid.bind_addr(port)
        _debug(f"bind_addr_compat(cmid, port={port}) ok")
        return
    except TypeError:
        pass

    if addrinfo_factory is None:
        from pyverbs.cmid import AddrInfo as addrinfo_factory
    if port_space is None:
        try:
            from pyverbs.cm_enums import RDMA_PS_TCP as port_space
        except Exception:
            port_space = None
    if passive_flag is None:
        try:
            from pyverbs.cm_enums import RAI_PASSIVE as passive_flag
        except Exception:
            passive_flag = 0
    try:
        from pyverbs.pyverbs_error import PyverbsRDMAError as err_cls
    except Exception:
        err_cls = RuntimeError

    candidates = [
        (None, None, port, None, port_space, passive_flag),
        ("0.0.0.0", None, port, None, port_space, passive_flag),
        (None, None, port, None, port_space, 0),
        ("0.0.0.0", None, port, None, port_space, 0),
    ]
    last_exc = None
    for args in candidates:
        try:
            ai = addrinfo_factory(*args)
            cmid.bind_addr(ai)
            _debug(f"bind_addr_compat(cmid, AddrInfo{args}) ok")
            return
        except err_cls as exc:
            last_exc = exc
    if last_exc is not None:
        raise last_exc


def bind_addr_src_compat(cmid, src_ip, src_port=None, addrinfo_factory=None, port_space=None):
    if addrinfo_factory is None:
        from pyverbs.cmid import AddrInfo as addrinfo_factory
    if port_space is None:
        try:
            from pyverbs.cm_enums import RDMA_PS_TCP as port_space
        except Exception:
            port_space = None
    if src_port is None:
        src_port = "0"
    ai = addrinfo_factory(src_ip, None, src_port, None, port_space, 0)
    cmid.bind_addr(ai)
    _debug(f"bind_addr_src_compat(cmid, src_ip={src_ip}, src_port={src_port}) ok")


def get_event_compat(ec):
    if hasattr(ec, "get_event"):
        return ec.get_event()
    if hasattr(ec, "get_cm_event"):
        return ec.get_cm_event()
    return None


def get_request_compat(listen_id, ec, sleep_fn=time.sleep, error_cls=None):
    ev = get_event_compat(ec)
    if ev is not None:
        return ev
    if not hasattr(listen_id, "get_request"):
        raise RuntimeError("No CM event API available for connect requests")
    if error_cls is None:
        try:
            from pyverbs.pyverbs_error import PyverbsRDMAError as error_cls
        except Exception:
            error_cls = RuntimeError
    while True:
        try:
            req = listen_id.get_request()
            _debug("get_request_compat(get_request) ok")
            return req
        except error_cls:
            sleep_fn(0.05)


def wait_established_compat(cmid, ec):
    ev = get_event_compat(ec)
    if ev is not None:
        return ev
    if hasattr(cmid, "establish"):
        cmid.establish()
        _debug("wait_established_compat(establish) ok")
        return None
    return None


def resolve_addr_compat(cmid, server_ip, port, timeout_ms, addrinfo_factory=None,
                        port_space=None, numerichost_flag=None, env=os.environ):
    try:
        cmid.resolve_addr(None, server_ip, port, timeout_ms)
        _debug(f"resolve_addr_compat(None, {server_ip}, {port}, {timeout_ms}) ok")
        return
    except TypeError:
        pass

    if addrinfo_factory is None:
        from pyverbs.cmid import AddrInfo as addrinfo_factory
    if port_space is None:
        from pyverbs.cm_enums import RDMA_PS_TCP as port_space
    if numerichost_flag is None:
        try:
            from pyverbs.cm_enums import RAI_NUMERICHOST as numerichost_flag
        except Exception:
            numerichost_flag = 0
    src = env.get("RDMA_SRC_IP")
    src_service = env.get("RDMA_SRC_PORT")
    if src is not None and src_service is None:
        src_service = "0"
    ai = addrinfo_factory(src, server_ip, src_service, port, port_space, numerichost_flag)
    cmid.resolve_addr(ai, timeout_ms)
    _debug(f"resolve_addr_compat(AddrInfo src={src}, dst={server_ip}, src_service={src_service}, dst_service={port}) ok")


def resolve_route_compat(cmid, timeout_ms, env=os.environ):
    if env.get("RDMA_FORCE_RESOLVE_ROUTE") != "1":
        return
    try:
        cmid.resolve_route(timeout_ms)
        _debug(f"resolve_route_compat({timeout_ms}) ok")
        return
    except TypeError:
        cmid.resolve_route()
        _debug("resolve_route_compat() ok")


def get_conn_param_compat(ec, cmid, connect_result, expected_event=None):
    ev = get_event_compat(ec)
    if ev is not None:
        if expected_event is not None and ev.event != expected_event:
            raise RuntimeError(f"Unexpected CM event: {ev.event}")
        connp = ev.param.conn
        ev.ack()
        return connp
    if connect_result is not None:
        if hasattr(connect_result, "private_data"):
            return connect_result
        if hasattr(connect_result, "param") and hasattr(connect_result.param, "conn"):
            return connect_result.param.conn
    if hasattr(cmid, "establish"):
        res = cmid.establish()
        _debug("get_conn_param_compat(establish) ok")
        if hasattr(res, "private_data"):
            return res
        if hasattr(res, "param") and hasattr(res.param, "conn"):
            return res.param.conn
    raise RuntimeError("No CM event API available to retrieve private_data")


def _list_devices(device_module=None):
    if device_module is None:
        try:
            import pyverbs.device as device_module
        except Exception:
            return []
    if hasattr(device_module, "DeviceList"):
        return device_module.DeviceList()
    if hasattr(device_module, "get_device_list"):
        return device_module.get_device_list()
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


def _open_context(dev, context_cls=None):
    if context_cls is None:
        from pyverbs.device import Context as context_cls
    name = _device_name(dev)
    try:
        return context_cls(dev)
    except Exception:
        pass
    try:
        return context_cls(name)
    except Exception:
        pass
    for key in ("device_name", "dev_name", "device", "ibdev_name", "name"):
        try:
            return context_cls(**{key: name})
        except Exception:
            pass
    for method in ("open_device", "from_device"):
        if hasattr(context_cls, method):
            try:
                return getattr(context_cls, method)(dev)
            except Exception:
                try:
                    return getattr(context_cls, method)(name)
                except Exception:
                    pass
    return context_cls()


def _fallback_context(env=os.environ, device_module=None, context_cls=None):
    devs = _list_devices(device_module=device_module)
    if not devs:
        return None
    wanted = env.get("RDMA_DEVICE")
    if wanted:
        for dev in devs:
            if _device_name(dev) == wanted:
                return _open_context(dev, context_cls=context_cls)
    ctx = _open_context(devs[0], context_cls=context_cls)
    _debug(f"_fallback_context -> {ctx}")
    return ctx


def get_cmid_context(cmid, fallback_context_fn=None):
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
    if hasattr(cmid, "context"):
        try:
            ctx_val = cmid.context()
            if ctx_val is not None:
                return ctx_val
        except Exception:
            pass
    if hasattr(cmid, "context"):
        try:
            ctx_val = cmid.context
            if ctx_val is not None:
                return ctx_val
        except Exception:
            pass
    if hasattr(cmid, "context"):
        try:
            ctx_val = cmid.context(pd=None)
            if ctx_val is not None:
                return ctx_val
        except Exception:
            pass
    if fallback_context_fn is None:
        fallback_context_fn = _fallback_context
    return fallback_context_fn()


def get_cmid_pd(cmid):
    if hasattr(cmid, "pd"):
        try:
            pd_obj = cmid.pd()
        except TypeError:
            try:
                pd_obj = cmid.pd
            except Exception:
                pd_obj = None
        if pd_obj is not None:
            return pd_obj
    return None


def get_pd_context(pd):
    if pd is None:
        return None
    if hasattr(pd, "ctx"):
        ctx_obj = pd.ctx
        if callable(ctx_obj):
            return ctx_obj()
        return ctx_obj
    return None


def create_mr(pd, buf, access, mr_cls=None):
    if mr_cls is None:
        from pyverbs.mr import MR as mr_cls
    try:
        mr = mr_cls(pd, buf, access)
        _debug("create_mr(buffer) ok")
        return mr
    except Exception:
        mr = mr_cls(pd, len(buf), access)
        mr_write(mr, buf)
        _debug("create_mr(length) ok")
        return mr


def mr_write(mr, data):
    blob = bytes(data)
    ctypes.memmove(mr.buf, blob, len(blob))


def mr_read(mr, length):
    return ctypes.string_at(mr.buf, length)


def set_cq_on_qp_init_attr(init_attr, cq):
    for key in ("scq", "send_cq"):
        if hasattr(init_attr, key):
            try:
                setattr(init_attr, key, cq)
            except Exception:
                pass
    for key in ("rcq", "recv_cq"):
        if hasattr(init_attr, key):
            try:
                setattr(init_attr, key, cq)
            except Exception:
                pass


def build_qp_init_attr(qp_init_cls, qp_type, cq, cap, sq_sig_all, env=os.environ):
    kwargs = {"qp_type": qp_type, "sq_sig_all": sq_sig_all}
    if env.get("RDMA_SKIP_CAP") != "1":
        kwargs["cap"] = cap
    if env.get("RDMA_SKIP_CQ") != "1":
        kwargs["scq"] = cq
        kwargs["rcq"] = cq
    try:
        init_attr = qp_init_cls(**kwargs)
    except TypeError:
        kwargs.pop("scq", None)
        kwargs.pop("rcq", None)
        if env.get("RDMA_SKIP_CQ") != "1":
            kwargs["send_cq"] = cq
            kwargs["recv_cq"] = cq
        init_attr = qp_init_cls(**kwargs)

    if env.get("RDMA_SKIP_CQ") != "1":
        set_cq_on_qp_init_attr(init_attr, cq)
    return init_attr


def build_qp_init_attr_minimal(qp_init_cls, qp_type, cap, sq_sig_all, env=os.environ):
    kwargs = {"qp_type": qp_type}
    if cap is not None and env.get("RDMA_SKIP_CAP") != "1":
        kwargs["cap"] = cap
    if sq_sig_all is not None:
        kwargs["sq_sig_all"] = sq_sig_all
    try:
        return qp_init_cls(**kwargs)
    except TypeError:
        pass
    kwargs.pop("sq_sig_all", None)
    try:
        return qp_init_cls(**kwargs)
    except TypeError:
        pass
    kwargs.pop("cap", None)
    try:
        return qp_init_cls(**kwargs)
    except TypeError:
        return None


def build_qp_init_attr_empty(qp_init_cls):
    try:
        return qp_init_cls()
    except TypeError:
        return None


def build_qp_init_attr_ex(qp_type, cq, cap, sq_sig_all, env=os.environ):
    try:
        from pyverbs.qp import QPInitAttrEx
    except Exception:
        return None
    try:
        init_attr = QPInitAttrEx(
            qp_type=qp_type,
            scq=None if env.get("RDMA_SKIP_CQ") == "1" else cq,
            rcq=None if env.get("RDMA_SKIP_CQ") == "1" else cq,
            cap=None if env.get("RDMA_SKIP_CAP") == "1" else cap,
            sq_sig_all=sq_sig_all,
        )
    except TypeError:
        init_attr = QPInitAttrEx(
            qp_type=qp_type,
            send_cq=None if env.get("RDMA_SKIP_CQ") == "1" else cq,
            recv_cq=None if env.get("RDMA_SKIP_CQ") == "1" else cq,
            cap=None if env.get("RDMA_SKIP_CAP") == "1" else cap,
            sq_sig_all=sq_sig_all,
        )
    if env.get("RDMA_SKIP_CQ") != "1":
        set_cq_on_qp_init_attr(init_attr, cq)
    return init_attr


def ensure_qp_init_attr(init_attr, cq):
    set_cq_on_qp_init_attr(init_attr, cq)


def create_qp_compat(cmid, pd, init_attr):
    debug_level = os.environ.get("RDMA_DEBUG_QP")
    if debug_level in ("1", "2"):
        def _val(obj, name):
            try:
                return getattr(obj, name)
            except Exception:
                return None
        fields = ("qp_type", "scq", "rcq", "send_cq", "recv_cq", "cap", "sq_sig_all", "srq")
        snapshot = {name: _val(init_attr, name) for name in fields}
        print(f"[rdma_compat] create_qp init_attr={snapshot}", file=sys.stderr)
        print(f"[rdma_compat] create_qp pd={pd} cmid={cmid}", file=sys.stderr)
        pd_ctx = get_pd_context(pd)
        if debug_level == "2":
            cmid_ctx = None
            if hasattr(cmid, "ctx"):
                cmid_ctx = cmid.ctx
            if cmid_ctx is None and hasattr(cmid, "context"):
                ctx_obj = cmid.context
                cmid_ctx = ctx_obj() if callable(ctx_obj) else ctx_obj
            print(f"[rdma_compat] create_qp pd.ctx={pd_ctx} cmid.ctx={cmid_ctx}", file=sys.stderr)
            try:
                cmid_context_val = cmid.context() if hasattr(cmid, "context") else None
            except Exception as exc:
                cmid_context_val = f"<error: {exc}>"
            try:
                cmid_pd_val = cmid.pd() if hasattr(cmid, "pd") else None
            except Exception as exc:
                cmid_pd_val = f"<error: {exc}>"
            print(f"[rdma_compat] create_qp cmid.context()={cmid_context_val} cmid.pd()={cmid_pd_val}", file=sys.stderr)
            try:
                doc = cmid.create_qp.__doc__
            except Exception:
                doc = None
            if doc:
                print(f"[rdma_compat] create_qp doc={doc.strip()}", file=sys.stderr)
            try:
                init_doc = cmid.init_qp_attr.__doc__ if hasattr(cmid, "init_qp_attr") else None
            except Exception:
                init_doc = None
            if init_doc:
                print(f"[rdma_compat] init_qp_attr doc={init_doc.strip()}", file=sys.stderr)
        if debug_level == "2":
            cap = _val(init_attr, "cap")
            if cap is not None:
                cap_fields = ("max_send_wr", "max_recv_wr", "max_send_sge", "max_recv_sge", "max_inline_data")
                cap_snapshot = {name: _val(cap, name) for name in cap_fields}
                print(f"[rdma_compat] create_qp cap={cap_snapshot}", file=sys.stderr)
    errors = []
    skip_pd = os.environ.get("RDMA_QP_SKIP_PD") == "1"
    init_first = os.environ.get("RDMA_QP_INIT_ONLY_FIRST") == "1"
    attempts = [("pd+init", (pd, init_attr)), ("init", (init_attr,)), ("pd", (pd,)), ("none", ())]
    if skip_pd:
        attempts = [("init", (init_attr,)), ("none", ())]
    if init_first and not skip_pd:
        attempts = [("init", (init_attr,)), ("pd+init", (pd, init_attr)), ("pd", (pd,)), ("none", ())]
    for label, args in attempts:
        try:
            qp_obj = cmid.create_qp(*args)
            if qp_obj is None and hasattr(cmid, "qp"):
                qp_obj = cmid.qp
            if qp_obj is not None:
                return qp_obj
        except Exception as exc:
            errors.append(exc)
            if debug_level == "2":
                print(f"[rdma_compat] create_qp attempt={label} error={exc}", file=sys.stderr)
    if hasattr(init_attr, "qp_type"):
        minimal_attr = build_qp_init_attr_minimal(
            init_attr.__class__,
            init_attr.qp_type,
            getattr(init_attr, "cap", None),
            getattr(init_attr, "sq_sig_all", None),
        )
        if minimal_attr is not None:
            try:
                qp_obj = cmid.create_qp(minimal_attr)
                if qp_obj is None and hasattr(cmid, "qp"):
                    qp_obj = cmid.qp
                if qp_obj is not None:
                    return qp_obj
            except Exception as exc:
                errors.append(exc)
                if debug_level == "2":
                    print(f"[rdma_compat] create_qp attempt=minimal error={exc}", file=sys.stderr)
        empty_attr = build_qp_init_attr_empty(init_attr.__class__)
        if empty_attr is not None:
            try:
                qp_obj = cmid.create_qp(empty_attr)
                if qp_obj is None and hasattr(cmid, "qp"):
                    qp_obj = cmid.qp
                if qp_obj is not None:
                    return qp_obj
            except Exception as exc:
                errors.append(exc)
                if debug_level == "2":
                    print(f"[rdma_compat] create_qp attempt=empty error={exc}", file=sys.stderr)
    if hasattr(cmid, "init_qp_attr"):
        try:
            try:
                qp_init = cmid.init_qp_attr(pd)
            except TypeError:
                qp_init = cmid.init_qp_attr()
            for attr in ("qp_type", "scq", "rcq", "cap", "sq_sig_all", "srq"):
                if hasattr(init_attr, attr) and hasattr(qp_init, attr):
                    setattr(qp_init, attr, getattr(init_attr, attr))
            qp_obj = cmid.create_qp(qp_init)
            if qp_obj is None and hasattr(cmid, "qp"):
                qp_obj = cmid.qp
            if qp_obj is not None:
                return qp_obj
        except Exception as exc:
            errors.append(exc)
            if debug_level == "2":
                print(f"[rdma_compat] create_qp attempt=init_qp_attr error={exc}", file=sys.stderr)
    if os.environ.get("RDMA_USE_QP_INIT_ATTR_EX") == "1" and hasattr(init_attr, "qp_type"):
        ex_attr = build_qp_init_attr_ex(
            init_attr.qp_type,
            init_attr.send_cq if hasattr(init_attr, "send_cq") else init_attr.scq,
            init_attr.cap,
            init_attr.sq_sig_all,
        )
        if ex_attr is not None:
            try:
                qp_obj = cmid.create_qp(ex_attr)
                if qp_obj is None and hasattr(cmid, "qp"):
                    qp_obj = cmid.qp
                if qp_obj is not None:
                    return qp_obj
            except Exception as exc:
                errors.append(exc)
                if debug_level == "2":
                    print(f"[rdma_compat] create_qp attempt=init_attr_ex error={exc}", file=sys.stderr)
    for exc in errors:
        if not isinstance(exc, TypeError):
            raise exc
    raise errors[-1]

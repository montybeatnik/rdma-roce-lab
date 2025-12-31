import ctypes
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "examples", "py"))

import rdma_compat as rc


def test_device_name_bytes_decodes():
    class Dev:
        def name(self):
            return b"rxe0"

    assert rc._device_name(Dev()) == "rxe0"


def test_create_cmid_signature_fallback():
    class FakeCMID:
        def __init__(self, *args, **kwargs):
            self.args = args
            self.kwargs = kwargs
            if len(args) == 2 and not kwargs:
                raise TypeError("expected qp_init_attr")

    cmid = rc.create_cmid("ec", "ps", cmid_cls=FakeCMID)
    assert cmid.args == ("ec", None, "ps")


def test_bind_addr_compat_uses_addrinfo():
    created = {}

    class FakeAddrInfo:
        def __init__(self, *args):
            created["args"] = args

    class FakeCMID:
        def __init__(self):
            self.bound = None

        def bind_addr(self, *args):
            if len(args) != 1 or not isinstance(args[0], FakeAddrInfo):
                raise TypeError("expected AddrInfo")
            self.bound = args[0]

    cmid = FakeCMID()
    rc.bind_addr_compat(cmid, "7471", addrinfo_factory=FakeAddrInfo, port_space="ps", passive_flag=99)
    assert created["args"][2] == "7471"


def test_resolve_addr_compat_addrinfo_path():
    created = {}

    class FakeAddrInfo:
        def __init__(self, *args):
            created["args"] = args

    class FakeCMID:
        def resolve_addr(self, *args):
            if len(args) != 2:
                raise TypeError("expected rai, timeout")
            self.args = args

    cmid = FakeCMID()
    rc.resolve_addr_compat(cmid, "1.2.3.4", "7471", 2000, addrinfo_factory=FakeAddrInfo,
                           port_space="ps", numerichost_flag=7)
    assert isinstance(cmid.args[0], FakeAddrInfo)
    assert cmid.args[1] == 2000
    assert created["args"][1] == "1.2.3.4"


def test_resolve_route_compat_skip_and_force():
    class FakeCMID:
        def __init__(self):
            self.calls = []

        def resolve_route(self, *args):
            self.calls.append(args)
            if args:
                raise TypeError("no args")

    cmid = FakeCMID()
    rc.resolve_route_compat(cmid, 1000, env={})
    assert cmid.calls == []

    rc.resolve_route_compat(cmid, 1000, env={"RDMA_FORCE_RESOLVE_ROUTE": "1"})
    assert cmid.calls == [(1000,), ()]


def test_get_request_compat_retries():
    class Err(Exception):
        pass

    class FakeListen:
        def __init__(self):
            self.calls = 0

        def get_request(self):
            self.calls += 1
            if self.calls < 3:
                raise Err("not ready")
            return "ok"

    sleeps = {"count": 0}

    def sleeper(_):
        sleeps["count"] += 1

    ev = rc.get_request_compat(FakeListen(), ec=None, sleep_fn=sleeper, error_cls=Err)
    assert ev == "ok"
    assert sleeps["count"] == 2


def test_create_mr_fallback_and_rw():
    class FakeMR:
        def __init__(self, _pd, buf_or_len, _access):
            if isinstance(buf_or_len, (bytes, bytearray, memoryview)):
                raise TypeError("buffer not supported")
            self.buf = ctypes.create_string_buffer(buf_or_len)
            self.lkey = 1
            self.rkey = 2

    data = bytearray(b"hello")
    mr = rc.create_mr(pd=object(), buf=data, access=0, mr_cls=FakeMR)
    assert rc.mr_read(mr, len(data)) == b"hello"


def test_get_cmid_context_uses_fallback():
    class FakeCMID:
        pass

    ctx = rc.get_cmid_context(FakeCMID(), fallback_context_fn=lambda: "ctx")
    assert ctx == "ctx"


def test_get_cmid_pd_method():
    class FakeCMID:
        def pd(self):
            return "pd"

    assert rc.get_cmid_pd(FakeCMID()) == "pd"


def test_get_pd_context():
    class Ctx:
        pass

    class PD:
        def __init__(self):
            self.ctx = Ctx()

    pd = PD()
    assert isinstance(rc.get_pd_context(pd), Ctx)

def test_ensure_qp_init_attr_sets_cq():
    class InitAttr:
        scq = None
        rcq = None

    init_attr = InitAttr()
    rc.ensure_qp_init_attr(init_attr, "cq")
    assert init_attr.scq == "cq"
    assert init_attr.rcq == "cq"


def test_build_qp_init_attr_fallbacks():
    class InitAttr:
        def __init__(self, **kwargs):
            self.kwargs = kwargs
            self.scq = kwargs.get("scq")
            self.rcq = kwargs.get("rcq")

    init_attr = rc.build_qp_init_attr(InitAttr, 1, "cq", "cap", True)
    assert init_attr.kwargs["scq"] == "cq"
    assert init_attr.kwargs["rcq"] == "cq"

    class SendRecvOnly:
        def __init__(self, **kwargs):
            self.kwargs = kwargs
            self.scq = None
            self.rcq = None
            self.send_cq = None
            self.recv_cq = None

    init_attr = rc.build_qp_init_attr(SendRecvOnly, 1, "cq", "cap", True)
    rc.set_cq_on_qp_init_attr(init_attr, "cq")
    assert init_attr.send_cq == "cq"
    assert init_attr.recv_cq == "cq"


def test_set_cq_on_qp_init_attr_sets_all():
    class InitAttr:
        scq = None
        rcq = None
        send_cq = None
        recv_cq = None

    init_attr = InitAttr()
    rc.set_cq_on_qp_init_attr(init_attr, "cq")
    assert init_attr.scq == "cq"
    assert init_attr.rcq == "cq"
    assert init_attr.send_cq == "cq"
    assert init_attr.recv_cq == "cq"


def test_build_qp_init_attr_minimal_variants():
    class InitAttr:
        def __init__(self, **kwargs):
            self.kwargs = kwargs

    init_attr = rc.build_qp_init_attr_minimal(InitAttr, 1, "cap", True)
    assert init_attr.kwargs["qp_type"] == 1
    assert init_attr.kwargs["cap"] == "cap"


def test_build_qp_init_attr_empty():
    class InitAttr:
        def __init__(self):
            self.kwargs = {}

    init_attr = rc.build_qp_init_attr_empty(InitAttr)
    assert isinstance(init_attr, InitAttr)


def test_build_qp_init_attr_env_flags():
    class InitAttr:
        def __init__(self, **kwargs):
            self.kwargs = kwargs
            self.scq = kwargs.get("scq")
            self.rcq = kwargs.get("rcq")
            self.send_cq = kwargs.get("send_cq")
            self.recv_cq = kwargs.get("recv_cq")

    env = {"RDMA_SKIP_CQ": "1", "RDMA_SKIP_CAP": "1"}
    init_attr = rc.build_qp_init_attr(InitAttr, 1, "cq", "cap", True, env=env)
    assert "cap" not in init_attr.kwargs
    assert "scq" not in init_attr.kwargs
    assert "rcq" not in init_attr.kwargs

def test_create_qp_compat_signature_fallback():
    class FakeCMID:
        def __init__(self):
            self.calls = []
            self.qp = "qp"

        def create_qp(self, *args):
            self.calls.append(args)
            if len(args) == 2:
                raise TypeError("one arg only")
            if len(args) == 1:
                return None
            if len(args) == 0:
                return None

    cmid = FakeCMID()
    qp = rc.create_qp_compat(cmid, pd="pd", init_attr="attr")
    assert qp == "qp"
    assert cmid.calls == [("pd", "attr"), ("attr",)]


def test_create_qp_compat_init_qp_attr_error_prefers_create_qp():
    class FakeCMID:
        def create_qp(self, *args):
            raise ValueError("create_qp failed")

        def init_qp_attr(self, _pd=None):
            raise TypeError("an integer is required")

    try:
        rc.create_qp_compat(FakeCMID(), pd="pd", init_attr="attr")
    except Exception as exc:
        assert isinstance(exc, ValueError)


def test_create_qp_compat_surfaces_rdma_error():
    class RDMAError(Exception):
        pass

    class FakeCMID:
        def create_qp(self, *args):
            if len(args) < 2:
                raise RDMAError("EINVAL")
            raise TypeError("bad signature")

    try:
        rc.create_qp_compat(FakeCMID(), pd="pd", init_attr="attr")
    except Exception as exc:
        assert isinstance(exc, RDMAError)

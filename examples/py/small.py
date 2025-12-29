from pyverbs.qp import QPCap, QPInitAttr, QPAttr, QP
from pyverbs.addr import GlobalRoute
from pyverbs.addr import AH, AHAttr
import pyverbs.device as d
from pyverbs.enums import ibv_qp_type
from pyverbs.pd import PD
from rdma_compat import create_mr, mr_write
from pyverbs.cq import CQ
import pyverbs.wr as pwr


ctx = d.Context(name='rxe0')
pd = PD(ctx)
cq = CQ(ctx, 100, None, None, 0)
cap = QPCap(100, 10, 1, 1, 0)
# import pdb
# pdb.set_trace()
qia = QPInitAttr(cap=cap, qp_type = ibv_qp_type.IBV_QPT_UD, scq=cq, rcq=cq)
# A UD QP will be in RTS if a QPAttr object is provided
udqp = QP(pd, qia, QPAttr())
port_num = 1
gid_index = 2 # Hard-coded for RoCE v2 interface

###DEBUG###
for i in range(8):
    try:
        gid = ctx.query_gid(1, i)
        print(i, gid)
    except Exception:
        pass
#Produces
# 0 fe80:0000:0000:0000:5054:00ff:fe8d:b24d
# 1 0000:0000:0000:0000:0000:ffff:c0a8:0222
# 2 fd38:cdd7:733e:a1da:5054:00ff:fe8d:b24d
# 3 0000:0000:0000:0000:0000:0000:0000:0000
# 4 0000:0000:0000:0000:0000:0000:0000:0000
# 5 0000:0000:0000:0000:0000:0000:0000:0000
# 6 0000:0000:0000:0000:0000:0000:0000:0000
# 7 0000:0000:0000:0000:0000:0000:0000:0000
###DEBUG###

gid = ctx.query_gid(port_num, gid_index)
print(f"{gid=}")
gr = GlobalRoute(dgid=gid, sgid_index=gid_index)
ah_attr = AHAttr(gr=gr, is_global=1, port_num=port_num, sl=0)
ah=AH(pd=pd, attr=ah_attr)
buf = bytearray(b"hello-ud")
mr = create_mr(pd, buf, 0)
mr_write(mr, buf)
sge = pwr.SGE(addr=mr.buf, length=len(buf), lkey=mr.lkey)
try:
    opcode = pwr.SendWR.SEND
except AttributeError:
    try:
        from pyverbs.enums import IBV_WR_SEND as opcode
    except Exception:
        opcode = 0
wr = pwr.SendWR(num_sge=1, sg=[sge], opcode=opcode)
wr.set_wr_ud(ah, 0x1101, 0) # in real life, use real values
udqp.post_send(wr)

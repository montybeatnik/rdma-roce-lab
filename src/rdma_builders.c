
#include "rdma_builders.h"

int build_pd_cq_qp(rdma_ctx *c, enum ibv_qp_type qpt, int cq_depth,
                   int max_send_wr, int max_recv_wr, int max_sge)
{
  if (c->qp)
  {
    LOG("QP already exists (qpn=%u); skipping build", c->qp->qp_num);
    return 0;
  }
  c->pd = ibv_alloc_pd(c->id->verbs);
  CHECK(!c->pd, "ibv_alloc_pd");
  c->cq = ibv_create_cq(c->id->verbs, cq_depth, NULL, NULL, 0);
  CHECK(!c->cq, "ibv_create_cq");
  struct ibv_qp_init_attr qa = {
      .send_cq = c->cq, .recv_cq = c->cq, .cap = {.max_send_wr = max_send_wr, .max_recv_wr = max_recv_wr, .max_send_sge = max_sge, .max_recv_sge = max_sge}, .qp_type = qpt};
  CHECK(rdma_create_qp(c->id, c->pd, &qa), "rdma_create_qp");
  c->qp = c->id->qp;
  dump_qp(c->qp);
  return 0;
}
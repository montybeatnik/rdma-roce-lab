/**
 * File: rdma_builders.c
 * Purpose: Construct PD/CQ/QP and dump helpers that inspect objects for
 * debugging.
 *
 * Overview:
 * Creates PD, CQ, and QP bound to an rdma_cm_id; associates CQ with both
 * send/recv; prints useful attributes like QP number, state, and caps to ease
 * troubleshooting.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally
 * verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.456505Z
 */

#include "rdma_builders.h"
/**
 * build_pd_cq_qp(rdma_ctx *c, enum ibv_qp_type qpt, int cq_depth,int
 * max_send_wr, int max_recv_wr, int max_sge) Creates or configures a verbs
 * object (PD/CQ/QP).
 *
 * Parameters:
 *   rdma_ctx *c - see function body for usage.
 *   enum ibv_qp_type qpt - see function body for usage.
 *   int cq_depth - see function body for usage.
 *   int max_send_wr - see function body for usage.
 *   int max_recv_wr - see function body for usage.
 *   int max_sge - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int build_pd_cq_qp(rdma_ctx *c, enum ibv_qp_type qpt, int cq_depth, int max_send_wr, int max_recv_wr, int max_sge)
{
    int err = 0;
    if (c->qp)
    {
        LOG("QP already exists (qpn=%u); skipping build", c->qp->qp_num);
        return 0;
    }
    c->pd = ibv_alloc_pd(c->id->verbs);
    if (!c->pd)
        return err_errno("ibv_alloc_pd");
    c->cq = ibv_create_cq(c->id->verbs, cq_depth, NULL, NULL, 0);
    if (!c->cq)
        return err_errno("ibv_create_cq");
    struct ibv_qp_init_attr qa = {.send_cq = c->cq,
                                  .recv_cq = c->cq,
                                  .cap = {.max_send_wr = max_send_wr,
                                          .max_recv_wr = max_recv_wr,
                                          .max_send_sge = max_sge,
                                          .max_recv_sge = max_sge},
                                  .qp_type = qpt};
    err = rdma_create_qp(c->id, c->pd, &qa);
    if (err)
        return err_errno("rdma_create_qp");
    c->qp = c->id->qp;
    dump_qp(c->qp);
    return 0;
}

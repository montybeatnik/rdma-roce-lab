
#pragma once
#include "rdma_ctx.h"
#include "common.h"

int build_pd_cq_qp(rdma_ctx *c, enum ibv_qp_type qpt, int cq_depth,
                   int max_send_wr, int max_recv_wr, int max_sge);


#pragma once
#include "common.h"
#include "rdma_ctx.h"

int build_pd_cq_qp(rdma_ctx *c, enum ibv_qp_type qpt, int cq_depth, int max_send_wr, int max_recv_wr, int max_sge);

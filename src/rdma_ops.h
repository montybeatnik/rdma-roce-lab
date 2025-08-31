
#pragma once
#include "rdma_ctx.h"
#include "common.h"

int post_write(struct ibv_qp *qp, struct ibv_mr *mr_src, void *src,
               uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled);
int post_read(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst,
              uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled);
int post_recv(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, size_t len, uint64_t wr_id);
int poll_one(struct ibv_cq *cq, struct ibv_wc *wc_out);

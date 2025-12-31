/**
 * File: rdma_ops.h
 * Purpose: Prototypes for posting RDMA operations and CQ polling.
 *
 * Overview:
 * Wraps verbs calls to post WRITE/READ/SEND/RECV and a CQ polling helper that prints WC fields. Keeps data-path logic
 * tidy in main programs.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.456115Z
 */

#pragma once
#include "common.h"
#include "rdma_ctx.h"
/* prototype */

int post_write(struct ibv_qp *qp, struct ibv_mr *mr_src, void *src, uint64_t remote_addr, uint32_t rkey, size_t len,
               uint64_t wr_id, int signaled);
/* prototype */
int post_read(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, uint64_t remote_addr, uint32_t rkey, size_t len,
              uint64_t wr_id, int signaled);
/* prototype */
int post_recv(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, size_t len, uint64_t wr_id);
/* prototype */
int poll_one(struct ibv_cq *cq, struct ibv_wc *wc_out);

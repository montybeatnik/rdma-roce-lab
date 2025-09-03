/**
 * File: rdma_ops.c
 * Purpose: Implementation of RDMA post operations (WRITE/READ/RECV) and CQ polling.
 *
 * Overview:
 * Wraps verbs calls to post WRITE/READ/SEND/RECV and a CQ polling helper that prints WC fields. Keeps data-path logic tidy in main programs.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.457546Z
 */


#include "rdma_ops.h"
/**
 * post_write(struct ibv_qp *qp, struct ibv_mr *mr_src, void *src,                uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled)
 * Auto-comment: Posts an RDMA work request (WQE) to the QP's send/recv queue.
 *
 * Parameters:
 *   struct ibv_qp *qp - see function body for usage.
 *   struct ibv_mr *mr_src - see function body for usage.
 *   void *src - see function body for usage.
 *   uint64_t remote_addr - see function body for usage.
 *   uint32_t rkey - see function body for usage.
 *   size_t len - see function body for usage.
 *   uint64_t wr_id - see function body for usage.
 *   int signaled - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int post_write(struct ibv_qp *qp, struct ibv_mr *mr_src, void *src,
               uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled) {
  struct ibv_sge s = {.addr=(uintptr_t)src, .length=(uint32_t)len, .lkey=mr_src->lkey};
  struct ibv_send_wr wr = {
    .wr_id=wr_id, .sg_list=&s, .num_sge=1,
    .opcode=IBV_WR_RDMA_WRITE,
    .send_flags= signaled ? IBV_SEND_SIGNALED : 0,
    .wr.rdma = {.remote_addr=remote_addr, .rkey=rkey}
  }, *bad = NULL;
  dump_sge(&s, "WRITE");
  dump_wr_rdma(&wr);
  return /* Post a SEND/WRITE/READ WQE to SQ */ ibv_post_send(qp, &wr, &bad);
}
/**
 * post_read(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst,               uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled)
 * Auto-comment: Posts an RDMA work request (WQE) to the QP's send/recv queue.
 *
 * Parameters:
 *   struct ibv_qp *qp - see function body for usage.
 *   struct ibv_mr *mr_dst - see function body for usage.
 *   void *dst - see function body for usage.
 *   uint64_t remote_addr - see function body for usage.
 *   uint32_t rkey - see function body for usage.
 *   size_t len - see function body for usage.
 *   uint64_t wr_id - see function body for usage.
 *   int signaled - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int post_read(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst,
              uint64_t remote_addr, uint32_t rkey, size_t len, uint64_t wr_id, int signaled) {
  struct ibv_sge s = {.addr=(uintptr_t)dst, .length=(uint32_t)len, .lkey=mr_dst->lkey};
  struct ibv_send_wr wr = {
    .wr_id=wr_id, .sg_list=&s, .num_sge=1,
    .opcode=IBV_WR_RDMA_READ,
    .send_flags= signaled ? IBV_SEND_SIGNALED : 0,
    .wr.rdma = {.remote_addr=remote_addr, .rkey=rkey}
  }, *bad = NULL;
  dump_sge(&s, "READ");
  dump_wr_rdma(&wr);
  return /* Post a SEND/WRITE/READ WQE to SQ */ ibv_post_send(qp, &wr, &bad);
}
/**
 * post_recv(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, size_t len, uint64_t wr_id)
 * Auto-comment: Posts an RDMA work request (WQE) to the QP's send/recv queue.
 *
 * Parameters:
 *   struct ibv_qp *qp - see function body for usage.
 *   struct ibv_mr *mr_dst - see function body for usage.
 *   void *dst - see function body for usage.
 *   size_t len - see function body for usage.
 *   uint64_t wr_id - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int post_recv(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, size_t len, uint64_t wr_id) {
  struct ibv_sge s = {.addr=(uintptr_t)dst, .length=(uint32_t)len, .lkey=mr_dst->lkey};
  struct ibv_recv_wr wr = {.wr_id=wr_id, .sg_list=&s, .num_sge=1}, *bad=NULL;
  dump_sge(&s, "RECV");
  return /* Post a RECV WQE to RQ */ ibv_post_recv(qp, &wr, &bad);
}
/**
 * poll_one(struct ibv_cq *cq, struct ibv_wc *wc_out)
 * Auto-comment: Polls the completion queue and returns one CQE.
 *
 * Parameters:
 *   struct ibv_cq *cq - see function body for usage.
 *   struct ibv_wc *wc_out - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int poll_one(struct ibv_cq *cq, struct ibv_wc *wc_out) {
  struct ibv_wc wc; int n;
  do { n = /* Poll CQ for completions */ ibv_poll_cq(cq, 1, &wc); } while (n == 0);
  if (n < 0 || wc.status != IBV_WC_SUCCESS) return -1;
  dump_wc(&wc);
  if (wc_out) *wc_out = wc;
  return 0;
}

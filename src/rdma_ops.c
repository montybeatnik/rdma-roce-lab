
#include "rdma_ops.h"

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
  return ibv_post_send(qp, &wr, &bad);
}

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
  return ibv_post_send(qp, &wr, &bad);
}

int post_recv(struct ibv_qp *qp, struct ibv_mr *mr_dst, void *dst, size_t len, uint64_t wr_id) {
  struct ibv_sge s = {.addr=(uintptr_t)dst, .length=(uint32_t)len, .lkey=mr_dst->lkey};
  struct ibv_recv_wr wr = {.wr_id=wr_id, .sg_list=&s, .num_sge=1}, *bad=NULL;
  dump_sge(&s, "RECV");
  return ibv_post_recv(qp, &wr, &bad);
}

int poll_one(struct ibv_cq *cq, struct ibv_wc *wc_out) {
  struct ibv_wc wc; int n;
  do { n = ibv_poll_cq(cq, 1, &wc); } while (n == 0);
  if (n < 0 || wc.status != IBV_WC_SUCCESS) return -1;
  dump_wc(&wc);
  if (wc_out) *wc_out = wc;
  return 0;
}

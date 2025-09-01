
#include "rdma_mem.h"

int alloc_and_reg(rdma_ctx *c, void **buf, struct ibv_mr **mr, size_t len,
                  int access_flags) {
  void *p = NULL;
  CHECK(posix_memalign(&p, 4096, len), "posix_memalign");
  memset(p, 0, len);
  struct ibv_mr *m = ibv_reg_mr(c->pd, p, len, access_flags);
  CHECK(!m, "ibv_reg_mr");
  *buf = p;
  *mr = m;
  return 0;
}

void mem_free_all(rdma_ctx *c) {
  if (c->mr_tx) ibv_dereg_mr(c->mr_tx);
  if (c->mr_rx) ibv_dereg_mr(c->mr_rx);
  if (c->mr_remote) ibv_dereg_mr(c->mr_remote);
  free(c->buf_tx);
  free(c->buf_rx); /* server owns buf_remote */
}

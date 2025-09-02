/**
 * File: rdma_mem.c
 * Purpose: Memory helpers: allocate page-aligned buffers; register/deregister MRs.
 *
 * Overview:
 * Abstracts page-aligned allocation using posix_memalign and registers memory with proper access flags (LOCAL_WRITE/REMOTE_WRITE/REMOTE_READ). Provides helpers to free & deregister in the right order.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.457304Z
 */


#include "rdma_mem.h"
/**
 * alloc_and_reg(rdma_ctx *c, void **buf, struct ibv_mr **mr, size_t len,                   int access_flags)
 * Auto-comment: Registers a memory region with the RNIC and returns lkey/rkey.
 *
 * Parameters:
 *   rdma_ctx *c - see function body for usage.
 *   void **buf - see function body for usage.
 *   struct ibv_mr **mr - see function body for usage.
 *   size_t len - see function body for usage.
 *   int access_flags - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int alloc_and_reg(rdma_ctx *c, void **buf, struct ibv_mr **mr, size_t len,
                  int access_flags) {
  void *p = NULL;
  CHECK(posix_memalign(&p, 4096, len), "posix_memalign");
  memset(p, 0, len);
  struct ibv_mr *m = /* Register app buffer with RNIC; pins pages & gets lkey/rkey */ ibv_reg_mr(c->pd, p, len, access_flags);
  CHECK(!m, "ibv_reg_mr");
  *buf = p;
  *mr = m;
  return 0;
}
/**
 * mem_free_all(rdma_ctx *c)
 * Auto-comment: See body for details.
 *
 * Parameters:
 *   rdma_ctx *c - see function body for usage.
 * Returns:
 *   void (see return statements).
 */

void mem_free_all(rdma_ctx *c) {
  if (c->mr_tx) ibv_dereg_mr(c->mr_tx);
  if (c->mr_rx) ibv_dereg_mr(c->mr_rx);
  if (c->mr_remote) ibv_dereg_mr(c->mr_remote);
  free(c->buf_tx);
  free(c->buf_rx); /* server owns buf_remote */
}

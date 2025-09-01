
#pragma once
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <stdint.h>

typedef struct {
  // CM
  struct rdma_event_channel *ec;
  struct rdma_cm_id *id;

  // Verbs
  struct ibv_pd *pd;
  struct ibv_cq *cq;
  struct ibv_qp *qp;

  // Memory
  void *buf_tx, *buf_rx, *buf_remote;
  struct ibv_mr *mr_tx, *mr_rx, *mr_remote;

  // Remote info learned via CM
  uint64_t remote_addr;
  uint32_t remote_rkey;
} rdma_ctx;

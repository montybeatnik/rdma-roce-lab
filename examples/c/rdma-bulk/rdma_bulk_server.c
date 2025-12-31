/**
 * RDMA bulk server: expose a large buffer and wait for client RDMA WRITEs.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "rdma_builders.h"
#include "rdma_cm_helpers.h"
#include "rdma_ctx.h"
#include "rdma_mem.h"
#include "rdma_bulk_common.h"

#define DEFAULT_PORT "7471"

struct bulk_info {
  uint64_t addr;
  uint32_t rkey;
  uint64_t len;
} __attribute__((packed));

static struct bulk_info pack_bulk_info(uint64_t addr, uint32_t rkey,
                                       uint64_t len) {
  struct bulk_info info = {.addr = htonll_u64(addr),
                           .rkey = htonl(rkey),
                           .len = htonll_u64(len)};
  return info;
}

static double elapsed_sec(const struct timespec *start,
                          const struct timespec *end) {
  double s = (double)(end->tv_sec - start->tv_sec);
  double ns = (double)(end->tv_nsec - start->tv_nsec) / 1e9;
  return s + ns;
}

int main(int argc, char **argv) {
  const char *port = (argc >= 2) ? argv[1] : DEFAULT_PORT;
  const char *size_str = (argc >= 3) ? argv[2] : "1G";
  uint64_t total = parse_size_bytes(size_str);
  if (total == 0) {
    fprintf(stderr, "Usage: %s <port> <bytes|K|M|G>\n", argv[0]);
    return 1;
  }

  rdma_ctx c = {0};
  cm_create_channel_and_id(&c);
  cm_server_listen(&c, NULL, port);

  struct rdma_cm_event *ev = NULL;
  CHECK(cm_wait_event(&c, RDMA_CM_EVENT_CONNECT_REQUEST, &ev),
        "CONNECT_REQUEST");
  c.id = ev->id;
  rdma_ack_cm_event(ev);

  build_pd_cq_qp(&c, IBV_QPT_RC, 256, 128, 128, 1);

  alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, total,
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                    IBV_ACCESS_REMOTE_WRITE);
  memset(c.buf_remote, 0, (size_t)total);

  struct bulk_info info =
      pack_bulk_info((uintptr_t)c.buf_remote, c.mr_remote->rkey, total);
  cm_server_accept_with_priv(&c, &info, sizeof(info));

  CHECK(cm_wait_event(&c, RDMA_CM_EVENT_ESTABLISHED, &ev), "ESTABLISHED");
  rdma_ack_cm_event(ev);

  printf("RDMA bulk server exposed %" PRIu64 " bytes\n", total);
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);

  // Wait for disconnect to mark completion.
  while (rdma_get_cm_event(c.ec, &ev) == 0) {
    if (ev->event == RDMA_CM_EVENT_DISCONNECTED) {
      rdma_ack_cm_event(ev);
      break;
    }
    rdma_ack_cm_event(ev);
  }

  clock_gettime(CLOCK_MONOTONIC, &t1);
  double secs = elapsed_sec(&t0, &t1);
  double mib = (double)total / (1024.0 * 1024.0);
  printf("RDMA bulk server finished in %.3f s (%.2f MiB/s)\n", secs,
         mib / secs);

  if (c.mr_remote) ibv_dereg_mr(c.mr_remote);
  free(c.buf_remote);
  if (c.qp) rdma_destroy_qp(c.id);
  if (c.cq) ibv_destroy_cq(c.cq);
  if (c.pd) ibv_dealloc_pd(c.pd);
  if (c.id) rdma_destroy_id(c.id);
  if (c.ec) rdma_destroy_event_channel(c.ec);
  return 0;
}

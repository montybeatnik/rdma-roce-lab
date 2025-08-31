
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "rdma_ctx.h"
#include "rdma_cm_helpers.h"
#include "rdma_builders.h"
#include "rdma_mem.h"

#define BUF_SZ 4096

int main(int argc, char **argv) {
  const char *port = (argc >= 2) ? argv[1] : "7471";

  rdma_ctx c = {0};
  LOG("Create CM channel + listen");
  cm_create_channel_and_id(&c);
  cm_server_listen(&c, port);

  LOG("Wait for CONNECT_REQUEST");
  struct rdma_cm_event *ev;
  CHECK(cm_wait_event(&c, RDMA_CM_EVENT_CONNECT_REQUEST, &ev), "CONNECT_REQUEST");
  c.id = ev->id; rdma_ack_cm_event(ev);

  LOG("Build PD/CQ/QP");
  build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);

  LOG("Register remote-exposed MR");
  alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, BUF_SZ,
    IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
  strcpy((char*)c.buf_remote, "server-initial");
  dump_mr(c.mr_remote, "server", c.buf_remote, BUF_SZ);

  struct remote_buf_info info = {
    .addr = htonll_u64((uintptr_t)c.buf_remote),
    .rkey = htonl(c.mr_remote->rkey)
  };
  LOG("Accept with private_data (addr=%#lx rkey=0x%x)",
      (unsigned long)ntohll_u64(info.addr), ntohl(info.rkey));
  cm_server_accept_with_priv(&c, &info, sizeof(info));

  CHECK(cm_wait_event(&c, RDMA_CM_EVENT_ESTABLISHED, &ev), "ESTABLISHED");
  rdma_ack_cm_event(ev);
  dump_qp(c.qp);

  LOG("Give client time to WRITE…");
  sleep(2);
  LOG("After WRITE, buf='%s'", (char*)c.buf_remote);

  snprintf((char*)c.buf_remote, BUF_SZ, "server-says-hello-%ld", (long)getpid());
  LOG("Server updated buf for READ: '%s'", (char*)c.buf_remote);

  LOG("Press Enter to disconnect…");
  getchar();
  rdma_disconnect(c.id);

  mem_free_all(&c);
  if (c.qp) rdma_destroy_qp(c.id);
  if (c.cq) ibv_destroy_cq(c.cq);
  if (c.pd) ibv_dealloc_pd(c.pd);
  if (c.id) rdma_destroy_id(c.id);
  if (c.ec) rdma_destroy_event_channel(c.ec);
  LOG("Done");
  return 0;
}

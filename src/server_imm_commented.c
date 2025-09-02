/**
 * File: server_imm.c
 * Purpose: Server that posts a RECV and waits for WRITE_WITH_IMM (immediate data).
 *
 * Overview:
 * Demonstrates WRITE_WITH_IMM (client) and a matching RECV (server) to signal application-level events without a separate SEND/SEND-with-imm path.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.458600Z
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "rdma_ctx.h"
#include "rdma_cm_helpers.h"
#include "rdma_builders.h"
#include "rdma_mem.h"
#include "rdma_ops.h"

#define BUF_SZ 4096
/**
 * main(int argc, char **argv)
 * Auto-comment: See body for details.
 *
 * Parameters:
 *   int argc - see function body for usage.
 *   char **argv - see function body for usage.
 * Returns:
 *   int (see return statements).
 */

int main(int argc, char **argv) {
  const char *port = (argc >= 2) ? argv[1] : "7472";

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

  // Post a RECV to receive the Write-With-Immediate notification
  LOG("Register and post RECV buffer (notification-only)");
  void *rx_note = NULL; struct ibv_mr *mr_rx = NULL;
  alloc_and_reg(&c, &rx_note, &mr_rx, 64, IBV_ACCESS_LOCAL_WRITE);
  CHECK(post_recv(c.qp, mr_rx, rx_note, 64, 100), "post_recv");

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

  LOG("Wait for RECV (WRITE_WITH_IMM notification)…");
  struct ibv_wc wc;
  CHECK(poll_one(c.cq, &wc), "poll recv");
  if (wc.opcode == IBV_WC_RECV_RDMA_WITH_IMM || (wc.wc_flags & IBV_WC_WITH_IMM)) {
    LOG("Got RECV with IMM: imm_data=0x%x (host=%u)", wc.imm_data, ntohl(wc.imm_data));
  } else {
    LOG("Got RECV without IMM: opcode=%d", wc.opcode);
  }

  LOG("After client's WRITE_WITH_IMM, buf='%s'", (char*)c.buf_remote);

  rdma_disconnect(c.id);

  // Cleanup
  if (mr_rx) ibv_dereg_mr(mr_rx);
  free(rx_note);
  mem_free_all(&c);
  if (c.qp) rdma_destroy_qp(c.id);
  if (c.cq) ibv_destroy_cq(c.cq);
  if (c.pd) ibv_dealloc_pd(c.pd);
  if (c.id) rdma_destroy_id(c.id);
  if (c.ec) rdma_destroy_event_channel(c.ec);
  LOG("Done");
  return 0;
}

/**
 * File: client_main.c
 * Purpose: Client that connects, RDMA WRITEs to remote buffer and RDMA READs it
 * back.
 *
 * Overview:
 * Resolves, builds PD/CQ/QP, connects, reads server's private_data (addr,rkey),
 * posts WRITE then READ, and logs completions. Demonstrates one-sided
 * operations end-to-end.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally
 * verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.458249Z
 */

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "rdma_builders.h"
#include "rdma_cm_helpers.h"
#include "rdma_ctx.h"
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
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
    return 1;
  }
  const char *ip = argv[1];
  const char *port = argv[2];

  rdma_ctx c = {0}; // initialize RDMA context to zero values. 
  LOG("Create CM channel + ID + connect");

  cm_create_channel_and_id(&c);
  CHECK(cm_client_resolve(&c, ip, port), "resolve");

  // Build PD/CQ/QP **before** rdma_connect
  LOG("Build PD/CQ/QP");
  build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);

  // Now connect (tiny credits for rxe)
  CHECK(cm_client_connect_only(&c, 1, 1), "rdma_connect");

  // Wait for CONNECTED (handles CONNECT_RESPONSE -> ESTABLISHED, and returns
  // conn params)
  struct rdma_conn_param connp = {0};
  CHECK(cm_wait_connected(&c, &connp), "ESTABLISHED");

  // Extract private_data safely into a local struct
  struct remote_buf_info info = {0};
  if (connp.private_data && connp.private_data_len >= sizeof(info)) {
    memcpy(&info, connp.private_data, sizeof(info));
  } else {
    fprintf(stderr, "No or short private_data\n");
    return 2;
  }

  c.remote_addr = ntohll_u64(info.addr);
  c.remote_rkey = ntohl(info.rkey);
  LOG("Got remote addr=%#lx rkey=0x%x", (unsigned long)c.remote_addr,
      c.remote_rkey);

  LOG("Register local tx/rx");
  alloc_and_reg(&c, &c.buf_tx, &c.mr_tx, BUF_SZ, IBV_ACCESS_LOCAL_WRITE);
  alloc_and_reg(&c, &c.buf_rx, &c.mr_rx, BUF_SZ, IBV_ACCESS_LOCAL_WRITE);
  strcpy((char *)c.buf_tx, "client-wrote-this");

  LOG("Post RDMA_WRITE");
  CHECK(post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                   strlen((char *)c.buf_tx) + 1, 1, 1),
        "post_write");
  struct ibv_wc wc;
  CHECK(poll_one(c.cq, &wc), "poll write");
  LOG("WRITE complete");

  LOG("Post RDMA_READ");
  CHECK(post_read(c.qp, c.mr_rx, c.buf_rx, c.remote_addr, c.remote_rkey, BUF_SZ,
                  2, 1),
        "post_read");
  CHECK(poll_one(c.cq, &wc), "poll read");
  LOG("READ complete: '%s'", (char *)c.buf_rx);

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

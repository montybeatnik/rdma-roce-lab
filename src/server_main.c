/**
 * File: server_main.c
 * Purpose: Server that exposes a remote buffer and accepts a connection
 * (READ/WRITE path).
 *
 * Overview:
 * Sets up CM listen → accept; builds PD/CQ/QP; registers an MR; shares
 * {addr,rkey} in private_data; then idles or exits after completions depending
 * on example scope.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally
 * verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.457955Z
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "rdma_builders.h"
#include "rdma_cm_helpers.h"
#include "rdma_ctx.h"
#include "rdma_mem.h"

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

int main(int argc, char **argv)
{
    const char *port = (argc >= 2) ? argv[1] : "7471";
    const char *bind_ip = getenv("RDMA_BIND_IP");

    rdma_ctx c = {0};
    LOGF("SLOW", "create CM channel + listen");
    cm_create_channel_and_id(&c);
    cm_server_listen(&c, bind_ip, port);

    LOGF("SLOW", "wait CONNECT_REQUEST");
    struct rdma_cm_event *ev;
    CHECK(cm_wait_event(&c, RDMA_CM_EVENT_CONNECT_REQUEST, &ev), "CONNECT_REQUEST");
    c.id = ev->id;
    rdma_ack_cm_event(ev);

    LOGF("SLOW", "build PD/CQ/QP");
    build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);

    LOGF("SLOW", "register remote-exposed MR");
    alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, BUF_SZ,
                  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    strcpy((char *)c.buf_remote, "server-initial");
    dump_mr(c.mr_remote, "server", c.buf_remote, BUF_SZ);

    struct remote_buf_info info = pack_remote_buf_info((uintptr_t)c.buf_remote, c.mr_remote->rkey);
    LOGF("SLOW", "accept with private_data");
    LOGF("SLOW", "  addr=%#lx", (unsigned long)(uintptr_t)c.buf_remote);
    LOGF("SLOW", "  rkey=0x%x", c.mr_remote->rkey);
    cm_server_accept_with_priv(&c, &info, sizeof(info));

    LOGF("SLOW", "wait ESTABLISHED");
    CHECK(cm_wait_event(&c, RDMA_CM_EVENT_ESTABLISHED, &ev), "ESTABLISHED");
    rdma_ack_cm_event(ev);
    dump_qp(c.qp);

    LOGF("FAST", "wait for client RDMA WRITE/READ");
    sleep(2);
    LOGF("DATA", "after client ops, buf='%s'", (char *)c.buf_remote);

    LOG("Press Enter to disconnect…");
    getchar();
    rdma_disconnect(c.id);

    mem_free_all(&c);
    if (c.qp)
        rdma_destroy_qp(c.id);
    if (c.cq)
        ibv_destroy_cq(c.cq);
    if (c.pd)
        ibv_dealloc_pd(c.pd);
    if (c.id)
        rdma_destroy_id(c.id);
    if (c.ec)
        rdma_destroy_event_channel(c.ec);
    LOG("Done");
    return 0;
}

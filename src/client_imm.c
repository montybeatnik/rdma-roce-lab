/**
 * File: client_imm.c
 * Purpose: Client that performs RDMA WRITE_WITH_IMM to notify remote server.
 *
 * Overview:
 * Demonstrates WRITE_WITH_IMM (client) and a matching RECV (server) to signal application-level events without a
 * separate SEND/SEND-with-imm path.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.459253Z
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
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

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server-ip> <port>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    const char *port = argv[2];
    const char *src_ip = getenv("RDMA_SRC_IP");
    uint8_t initiator_depth = 0;
    uint8_t responder_resources = 0;
    const char *init_env = getenv("RDMA_INITIATOR_DEPTH");
    const char *resp_env = getenv("RDMA_RESPONDER_RESOURCES");
    if (init_env && *init_env)
        initiator_depth = (uint8_t)strtoul(init_env, NULL, 10);
    if (resp_env && *resp_env)
        responder_resources = (uint8_t)strtoul(resp_env, NULL, 10);

    // --- connect & setup (new flow) ---
    rdma_ctx c = {0};
    LOGF("SLOW", "create CM channel + ID");
    cm_create_channel_and_id(&c);

    // 1) Resolve first (ADDR/ROUTE)
    LOGF("SLOW", "resolve %s:%s", ip, port);
    LOGF("SLOW", "  src_ip=%s", src_ip ? src_ip : "default");
    CHECK(cm_client_resolve(&c, ip, port, src_ip), "resolve");

    // 2) Build PD/CQ/QP BEFORE rdma_connect
    LOGF("SLOW", "build PD/CQ/QP");
    build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);

    // 3) rdma_connect with tiny credits (SoftRoCE-friendly)
    LOGF("SLOW", "rdma_connect");
    LOGF("SLOW", "  initiator_depth=%u", initiator_depth);
    LOGF("SLOW", "  responder_resources=%u", responder_resources);
    CHECK(cm_client_connect_only(&c, initiator_depth, responder_resources), "rdma_connect");

    // 4) Wait for ESTABLISHED and pull private_data safely
    struct rdma_conn_param connp = {0};
    LOGF("SLOW", "wait ESTABLISHED");
    CHECK(cm_wait_connected(&c, &connp), "ESTABLISHED");

    struct remote_buf_info info = {0};
    LOGF("SLOW", "read private_data (remote addr/rkey)");
    if (connp.private_data && connp.private_data_len >= sizeof(info))
    {
        memcpy(&info, connp.private_data, sizeof(info));
    }
    else
    {
        fprintf(stderr, "No private_data\n");
        return 2;
    }
    unpack_remote_buf_info(&info, &c.remote_addr, &c.remote_rkey);
    LOGF("SLOW", "remote addr=%#lx", (unsigned long)c.remote_addr);
    LOGF("SLOW", "remote rkey=0x%x", c.remote_rkey);

    LOGF("FAST", "register local TX");
    alloc_and_reg(&c, &c.buf_tx, &c.mr_tx, BUF_SZ, IBV_ACCESS_LOCAL_WRITE);
    strcpy((char *)c.buf_tx, "client-wrote-with-imm");

    // RDMA_WRITE_WITH_IMM: include imm_data (e.g., payload length or tag)
    uint32_t imm = htonl((uint32_t)(strlen((char *)c.buf_tx) + 1));

    struct ibv_sge s = {
        .addr = (uintptr_t)c.buf_tx, .length = (uint32_t)strlen((char *)c.buf_tx) + 1, .lkey = c.mr_tx->lkey};
    struct ibv_send_wr wr = {.wr_id = 1,
                             .sg_list = &s,
                             .num_sge = 1,
                             .opcode = IBV_WR_RDMA_WRITE_WITH_IMM,
                             .send_flags = IBV_SEND_SIGNALED,
                             .wr.rdma = {.remote_addr = c.remote_addr, .rkey = c.remote_rkey},
                             .imm_data = imm},
                       *bad = NULL;
    dump_sge(&s, "WRITE_WITH_IMM");
    dump_wr_rdma(&wr);
    LOGF("DATA", "post RDMA_WRITE_WITH_IMM len=%u", s.length);
    LOGF("DATA", "  imm_data host=%u", ntohl(imm));
    LOGF("DATA", "  imm_data net=0x%x", imm);
    CHECK(/* Post a SEND/WRITE/READ WQE to SQ */ ibv_post_send(c.qp, &wr, &bad), "ibv_post_send write_with_imm");

    struct ibv_wc wc;
    LOGF("DATA", "poll RDMA_WRITE_WITH_IMM");
    CHECK(poll_one(c.cq, &wc), "poll write_with_imm");
    LOGF("DATA", "RDMA_WRITE_WITH_IMM completed");

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

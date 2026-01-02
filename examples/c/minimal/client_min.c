/**
 * Minimal client example for tutorial walkthroughs.
 */

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

    rdma_ctx c = {0};
    LOGF("SLOW", "create CM channel + ID");
    cm_create_channel_and_id(&c);
    LOGF("SLOW", "resolve %s:%s", ip, port);
    if (src_ip && *src_ip)
    {
        LOGF("SLOW", "  src_ip=%s", src_ip);
    }
    CHECK(cm_client_resolve(&c, ip, port, src_ip), "resolve");

    LOGF("SLOW", "build PD/CQ/QP");
    build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);
    LOGF("SLOW", "rdma_connect");
    CHECK(cm_client_connect_only(&c, 1, 1), "rdma_connect");

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
        fprintf(stderr, "No or short private_data\n");
        return 2;
    }

    unpack_remote_buf_info(&info, &c.remote_addr, &c.remote_rkey);
    LOGF("SLOW", "remote addr=%#lx", (unsigned long)c.remote_addr);
    LOGF("SLOW", "remote rkey=0x%x", c.remote_rkey);

    LOGF("FAST", "register local TX/RX");
    alloc_and_reg(&c, &c.buf_tx, &c.mr_tx, BUF_SZ, IBV_ACCESS_LOCAL_WRITE);
    alloc_and_reg(&c, &c.buf_rx, &c.mr_rx, BUF_SZ, IBV_ACCESS_LOCAL_WRITE);
    strcpy((char *)c.buf_tx, "client-wrote-this");

    LOGF("DATA", "post RDMA_WRITE len=%zu", strlen((char *)c.buf_tx) + 1);
    CHECK(post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey, strlen((char *)c.buf_tx) + 1, 1, 1),
          "post_write");
    struct ibv_wc wc;
    LOGF("DATA", "poll RDMA_WRITE");
    CHECK(poll_one(c.cq, &wc), "poll write");

    LOGF("DATA", "post RDMA_READ len=%u", (unsigned)BUF_SZ);
    CHECK(post_read(c.qp, c.mr_rx, c.buf_rx, c.remote_addr, c.remote_rkey, BUF_SZ, 2, 1), "post_read");
    LOGF("DATA", "poll RDMA_READ");
    CHECK(poll_one(c.cq, &wc), "poll read");

    LOGF("DATA", "readback '%s'", (char *)c.buf_rx);
    printf("Client read back: '%s'\n", (char *)c.buf_rx);

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
    return 0;
}

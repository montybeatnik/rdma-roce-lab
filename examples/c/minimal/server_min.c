/**
 * Minimal server example for tutorial walkthroughs.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "rdma_builders.h"
#include "rdma_cm_helpers.h"
#include "rdma_ctx.h"
#include "rdma_mem.h"

#define BUF_SZ 4096

int main(int argc, char **argv)
{
    const char *port = (argc >= 2) ? argv[1] : "7471";

    rdma_ctx c = {0};
    cm_create_channel_and_id(&c);
    cm_server_listen(&c, port);

    struct rdma_cm_event *ev;
    CHECK(cm_wait_event(&c, RDMA_CM_EVENT_CONNECT_REQUEST, &ev), "CONNECT_REQUEST");
    c.id = ev->id;
    rdma_ack_cm_event(ev);

    build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1);

    alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, BUF_SZ,
                  IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    strcpy((char *)c.buf_remote, "server-initial");

    struct remote_buf_info info = pack_remote_buf_info((uintptr_t)c.buf_remote, c.mr_remote->rkey);
    cm_server_accept_with_priv(&c, &info, sizeof(info));

    CHECK(cm_wait_event(&c, RDMA_CM_EVENT_ESTABLISHED, &ev), "ESTABLISHED");
    rdma_ack_cm_event(ev);

    sleep(2);
    printf("Server buffer after client ops: '%s'\n", (char *)c.buf_remote);

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

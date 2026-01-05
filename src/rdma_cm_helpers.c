#include "rdma_cm_helpers.h"

#include <netdb.h>
#include <stdlib.h>
#include <string.h>

static int err_gai(const char *ctx, int rc)
{
    LOG_ERR("%s: %s", ctx, gai_strerror(rc));
    return -1;
}

int cm_create_channel_and_id(rdma_ctx *c)
{
    c->ec = rdma_create_event_channel();
    if (!c->ec)
        return err_errno("rdma_create_event_channel");
    if (rdma_create_id(c->ec, &c->id, NULL, RDMA_PS_TCP))
        return err_errno("rdma_create_id");
    return 0;
}

int cm_server_listen(rdma_ctx *c, const char *ip, const char *port)
{
    struct addrinfo hints = {0}, *res = NULL;
    int rc = 0;
    if (!ip || !*ip)
    {
        hints.ai_flags = AI_PASSIVE;
    }
    hints.ai_family = AF_INET;
    rc = getaddrinfo(ip, port, &hints, &res);
    if (rc)
        return err_gai("getaddrinfo", rc);
    if (rdma_bind_addr(c->id, res->ai_addr))
    {
        freeaddrinfo(res);
        return err_errno("rdma_bind_addr");
    }
    freeaddrinfo(res);
    if (rdma_listen(c->id, 1))
        return err_errno("rdma_listen");
    return 0;
}

int cm_wait_event(rdma_ctx *c, enum rdma_cm_event_type want, struct rdma_cm_event **out)
{
    struct rdma_cm_event *ev = NULL;
    if (rdma_get_cm_event(c->ec, &ev))
        return err_errno("rdma_get_cm_event");
    if (ev->event != want)
    {
        LOG("Unexpected CM event: got=%s(%d) want=%s(%d) status=%d", rdma_event_str(ev->event), ev->event,
            rdma_event_str(want), want, ev->status);
        rdma_ack_cm_event(ev);
        return -1;
    }
    *out = ev;
    return 0;
}

int cm_wait_connected(rdma_ctx *c, struct rdma_conn_param *out_conn_param)
{
    struct rdma_cm_event *ev = NULL;

    // Try ESTABLISHED directly
    if (rdma_get_cm_event(c->ec, &ev) == 0)
    {
        if (ev->event == RDMA_CM_EVENT_ESTABLISHED)
        {
            if (out_conn_param)
                *out_conn_param = ev->param.conn; // copy before ack
            rdma_ack_cm_event(ev);
            return 0;
        }
        // Not ESTABLISHED; remember what we saw and ack it
        int got = ev->event;
        int status = ev->status;
        rdma_ack_cm_event(ev);

        // If we got CONNECT_RESPONSE, wait one more for ESTABLISHED
        if (got == RDMA_CM_EVENT_CONNECT_RESPONSE)
        {
            if (rdma_get_cm_event(c->ec, &ev) == 0 && ev->event == RDMA_CM_EVENT_ESTABLISHED)
            {
                if (out_conn_param)
                    *out_conn_param = ev->param.conn;
                rdma_ack_cm_event(ev);
                return 0;
            }
        }

        // Unexpected sequence
        LOG("Unexpected CM event while waiting for ESTABLISHED: got=%s(%d) status=%d (%s)", rdma_event_str(got), got,
            status, strerror(status));
        return -1;
    }
    perror("rdma_get_cm_event");
    return -1;
}

int cm_server_accept_with_priv(rdma_ctx *c, const void *priv, size_t len)
{
    uint8_t responder_resources = 1;
    uint8_t initiator_depth = 1;
    const char *resp_env = getenv("RDMA_RESPONDER_RESOURCES");
    const char *init_env = getenv("RDMA_INITIATOR_DEPTH");
    if (resp_env && *resp_env)
        responder_resources = (uint8_t)strtoul(resp_env, NULL, 10);
    if (init_env && *init_env)
        initiator_depth = (uint8_t)strtoul(init_env, NULL, 10);
    struct rdma_conn_param p = {.private_data = priv,
                                .private_data_len = (uint8_t)len,
                                // for READ/WRITE example
                                .responder_resources = responder_resources,
                                .initiator_depth = initiator_depth,
                                // for IMM example
                                // .responder_resources = 0,
                                // .initiator_depth = 0,
                                .retry_count = 7,
                                .rnr_retry_count = 7};
    if (rdma_accept(c->id, &p))
        return err_errno("rdma_accept");
    return 0;
}

int cm_client_resolve(rdma_ctx *c, const char *ip, const char *port, const char *src_ip)
{
    struct addrinfo hints = {0}, *res = NULL, *src_res = NULL;
    int rc = 0;
    hints.ai_family = AF_INET;

    LOG("cm: getaddrinfo(%s:%s)", ip, port);
    rc = getaddrinfo(ip, port, &hints, &res);
    if (rc)
        return err_gai("getaddrinfo", rc);

    LOG("cm: rdma_resolve_addr(timeout=5000ms)");
    if (src_ip && *src_ip)
    {
        LOG("cm: getaddrinfo(src=%s)", src_ip);
        rc = getaddrinfo(src_ip, NULL, &hints, &src_res);
        if (rc)
        {
            freeaddrinfo(res);
            return err_gai("getaddrinfo src", rc);
        }
    }
    if (rdma_resolve_addr(c->id, src_res ? src_res->ai_addr : NULL, res->ai_addr, 5000))
    {
        if (src_res)
            freeaddrinfo(src_res);
        freeaddrinfo(res);
        return err_errno("rdma_resolve_addr");
    }
    if (src_res)
        freeaddrinfo(src_res);
    freeaddrinfo(res);

    struct rdma_cm_event *ev = NULL;

    LOG("cm: waiting ADDR_RESOLVED");
    if (cm_wait_event(c, RDMA_CM_EVENT_ADDR_RESOLVED, &ev))
        return -1;
    rdma_ack_cm_event(ev);

    LOG("cm: rdma_resolve_route(timeout=5000ms)");
    if (rdma_resolve_route(c->id, 5000))
        return err_errno("rdma_resolve_route");

    LOG("cm: waiting ROUTE_RESOLVED");
    if (cm_wait_event(c, RDMA_CM_EVENT_ROUTE_RESOLVED, &ev))
        return -1;
    rdma_ack_cm_event(ev);

    return 0;
}

int cm_client_connect_only(rdma_ctx *c, uint8_t initiator_depth, uint8_t responder_resources)
{
    struct rdma_conn_param p = {.initiator_depth = initiator_depth,
                                .responder_resources = responder_resources,
                                .retry_count = 7,
                                .rnr_retry_count = 7};
    LOG("cm: rdma_connect()");
    if (rdma_connect(c->id, &p))
        return err_errno("rdma_connect");
    LOG("cm: rdma_connect() returned");
    return 0;
}

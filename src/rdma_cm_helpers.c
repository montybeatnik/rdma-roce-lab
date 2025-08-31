
#include <string.h>
#include <netdb.h>
#include "rdma_cm_helpers.h"

int cm_create_channel_and_id(rdma_ctx *c)
{
  c->ec = rdma_create_event_channel();
  CHECK(!c->ec, "rdma_create_event_channel");
  CHECK(rdma_create_id(c->ec, &c->id, NULL, RDMA_PS_TCP), "rdma_create_id");
  return 0;
}

int cm_server_listen(rdma_ctx *c, const char *port)
{
  struct addrinfo hints = {0}, *res = NULL;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_INET;
  CHECK(getaddrinfo(NULL, port, &hints, &res), "getaddrinfo");
  CHECK(rdma_bind_addr(c->id, res->ai_addr), "rdma_bind_addr");
  freeaddrinfo(res);
  CHECK(rdma_listen(c->id, 1), "rdma_listen");
  return 0;
}

int cm_wait_event(rdma_ctx *c, enum rdma_cm_event_type want, struct rdma_cm_event **out)
{
  struct rdma_cm_event *ev = NULL;
  CHECK(rdma_get_cm_event(c->ec, &ev), "rdma_get_cm_event");
  if (ev->event != want)
  {
    LOG("Unexpected CM event: got=%d want=%d status=%d", ev->event, want, ev->status);
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
    LOG("Unexpected CM event while waiting for ESTABLISHED: got=%d status=%d", got, status);
    return -1;
  }
  perror("rdma_get_cm_event");
  return -1;
}

int cm_server_accept_with_priv(rdma_ctx *c, const void *priv, size_t len)
{
  struct rdma_conn_param p = {
      .private_data = priv, .private_data_len = (uint8_t)len, .responder_resources = 1, .initiator_depth = 1, .retry_count = 7, .rnr_retry_count = 7};
  CHECK(rdma_accept(c->id, &p), "rdma_accept");
  return 0;
}

int cm_client_resolve(rdma_ctx *c, const char *ip, const char *port)
{
  struct addrinfo hints = {0}, *res = NULL;
  hints.ai_family = AF_INET;

  LOG("cm: getaddrinfo(%s:%s)", ip, port);
  CHECK(getaddrinfo(ip, port, &hints, &res), "getaddrinfo");

  LOG("cm: rdma_resolve_addr(timeout=5000ms)");
  CHECK(rdma_resolve_addr(c->id, NULL, res->ai_addr, 5000), "rdma_resolve_addr");
  freeaddrinfo(res);

  struct rdma_cm_event *ev = NULL;

  LOG("cm: waiting ADDR_RESOLVED");
  CHECK(cm_wait_event(c, RDMA_CM_EVENT_ADDR_RESOLVED, &ev), "ADDR_RESOLVED");
  rdma_ack_cm_event(ev);

  LOG("cm: rdma_resolve_route(timeout=5000ms)");
  CHECK(rdma_resolve_route(c->id, 5000), "rdma_resolve_route");

  LOG("cm: waiting ROUTE_RESOLVED");
  CHECK(cm_wait_event(c, RDMA_CM_EVENT_ROUTE_RESOLVED, &ev), "ROUTE_RESOLVED");
  rdma_ack_cm_event(ev);

  return 0;
}

int cm_client_connect_only(rdma_ctx *c, uint8_t initiator_depth, uint8_t responder_resources)
{
  struct rdma_conn_param p = {
      .initiator_depth = initiator_depth,
      .responder_resources = responder_resources,
      .retry_count = 7,
      .rnr_retry_count = 7};
  LOG("cm: rdma_connect()");
  CHECK(rdma_connect(c->id, &p), "rdma_connect");
  LOG("cm: rdma_connect() returned");
  return 0;
}
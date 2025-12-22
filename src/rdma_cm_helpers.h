#pragma once
#include <rdma/rdma_cma.h>

#include "common.h"
#include "rdma_ctx.h"

int cm_create_channel_and_id(rdma_ctx *c);
int cm_server_listen(rdma_ctx *c, const char *ip, const char *port);
int cm_wait_event(rdma_ctx *c, enum rdma_cm_event_type want,
                  struct rdma_cm_event **out);

/* NEW: make sure this line exists */
int cm_wait_connected(rdma_ctx *c, struct rdma_conn_param *out_conn_param);

int cm_server_accept_with_priv(rdma_ctx *c, const void *priv, size_t len);
int cm_client_connect(rdma_ctx *c, const char *ip, const char *port);

// Fixes the connect issue
int cm_client_resolve(rdma_ctx *c, const char *ip, const char *port,
                      const char *src_ip);
int cm_client_connect_only(rdma_ctx *c, uint8_t initiator_depth,
                           uint8_t responder_resources);
int cm_wait_connected(rdma_ctx *c, struct rdma_conn_param *out_conn_param);

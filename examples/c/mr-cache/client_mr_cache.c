/**
 * MR cache client: reuse user-mode memory registrations keyed by size.
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

#define MAX_BUF (32 * 1024)

struct mr_cache_entry
{
    size_t size;
    void *buf;
    struct ibv_mr *mr;
    int in_use;
};

struct mr_cache
{
    rdma_ctx *ctx;
    struct mr_cache_entry *ents;
    size_t count;
    size_t cap;
    size_t hits;
    size_t misses;
    size_t regs;
};

static int mr_cache_grow(struct mr_cache *c)
{
    size_t next = c->cap ? c->cap * 2 : 8;
    struct mr_cache_entry *n = realloc(c->ents, next * sizeof(*n));
    if (!n)
        return err_errno("mr_cache realloc");
    memset(n + c->cap, 0, (next - c->cap) * sizeof(*n));
    c->ents = n;
    c->cap = next;
    return 0;
}

static int mr_cache_get(struct mr_cache *c, size_t size, void **buf, struct ibv_mr **mr)
{
    for (size_t i = 0; i < c->count; i++)
    {
        struct mr_cache_entry *e = &c->ents[i];
        if (!e->in_use && e->size == size)
        {
            e->in_use = 1;
            c->hits++;
            *buf = e->buf;
            *mr = e->mr;
            return 0;
        }
    }

    if (c->count == c->cap)
    {
        if (mr_cache_grow(c))
            return -1;
    }

    void *p = NULL;
    int rc = posix_memalign(&p, 4096, size);
    if (rc)
    {
        LOG_ERR("posix_memalign: %s", strerror(rc));
        return -1;
    }
    memset(p, 0, size);
    struct ibv_mr *m = ibv_reg_mr(c->ctx->pd, p, size, IBV_ACCESS_LOCAL_WRITE);
    if (!m)
    {
        free(p);
        return err_errno("ibv_reg_mr");
    }

    struct mr_cache_entry *e = &c->ents[c->count++];
    e->size = size;
    e->buf = p;
    e->mr = m;
    e->in_use = 1;

    c->misses++;
    c->regs++;

    *buf = p;
    *mr = m;
    return 0;
}

static void mr_cache_put(struct mr_cache *c, struct ibv_mr *mr)
{
    for (size_t i = 0; i < c->count; i++)
    {
        if (c->ents[i].mr == mr)
        {
            c->ents[i].in_use = 0;
            return;
        }
    }
}

static void mr_cache_cleanup(struct mr_cache *c)
{
    for (size_t i = 0; i < c->count; i++)
    {
        if (c->ents[i].mr)
            ibv_dereg_mr(c->ents[i].mr);
        free(c->ents[i].buf);
    }
    free(c->ents);
}

int main(int argc, char **argv)
{
    int err = 0;
    struct mr_cache cache = {0};
    int cache_inited = 0;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server-ip> <port> [iters]\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    const char *port = argv[2];
    int iters = (argc >= 4) ? atoi(argv[3]) : 40;
    if (iters <= 0)
        iters = 40;

    rdma_ctx c = {0};
    LOGF("SLOW", "create CM channel + ID");
    if (cm_create_channel_and_id(&c))
    {
        err = 1;
        goto cleanup;
    }
    LOGF("SLOW", "resolve %s:%s", ip, port);
    if (cm_client_resolve(&c, ip, port, NULL))
    {
        err = 1;
        goto cleanup;
    }

    LOGF("SLOW", "build PD/CQ/QP");
    if (build_pd_cq_qp(&c, IBV_QPT_RC, 64, 32, 32, 1))
    {
        err = 1;
        goto cleanup;
    }

    LOGF("SLOW", "rdma_connect");
    if (cm_client_connect_only(&c, 1, 1))
    {
        err = 1;
        goto cleanup;
    }

    struct rdma_conn_param connp = {0};
    LOGF("SLOW", "wait ESTABLISHED");
    if (cm_wait_connected(&c, &connp))
    {
        err = 1;
        goto cleanup;
    }

    struct remote_buf_info info = {0};
    if (connp.private_data && connp.private_data_len >= sizeof(info))
    {
        memcpy(&info, connp.private_data, sizeof(info));
    }
    else
    {
        fprintf(stderr, "No or short private_data\n");
        err = 2;
        goto cleanup;
    }
    unpack_remote_buf_info(&info, &c.remote_addr, &c.remote_rkey);
    LOGF("SLOW", "remote addr=%#lx rkey=0x%x", (unsigned long)c.remote_addr, c.remote_rkey);

    size_t sizes[] = {4096, 8192, 16384, 32768};
    int sizes_n = (int)(sizeof(sizes) / sizeof(sizes[0]));

    cache.ctx = &c;
    cache_inited = 1;

    LOGF("DATA", "starting %d iterations with MR cache", iters);
    for (int i = 0; i < iters; i++)
    {
        size_t sz = sizes[i % sizes_n];
        if (sz > MAX_BUF)
            sz = MAX_BUF;

        void *buf = NULL;
        struct ibv_mr *mr = NULL;
        if (mr_cache_get(&cache, sz, &buf, &mr))
        {
            err = 1;
            goto cleanup;
        }

        memset(buf, (int)(0x41 + (i % 26)), sz);
        if (post_write(c.qp, mr, buf, c.remote_addr, c.remote_rkey, sz, (uint64_t)i + 1, 1))
        {
            err = 1;
            goto cleanup;
        }

        struct ibv_wc wc;
        if (poll_one(c.cq, &wc))
        {
            err = 1;
            goto cleanup;
        }

        mr_cache_put(&cache, mr);
    }

    LOGF("DATA", "mr_cache hits=%zu misses=%zu regs=%zu entries=%zu",
         cache.hits, cache.misses, cache.regs, cache.count);

    rdma_disconnect(c.id);

cleanup:
    if (cache_inited)
        mr_cache_cleanup(&cache);
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
    return err;
}

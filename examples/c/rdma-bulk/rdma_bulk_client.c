/**
 * RDMA bulk client: write a large payload to remote memory in fixed chunks.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "rdma_builders.h"
#include "rdma_bulk_common.h"
#include "rdma_cm_helpers.h"
#include "rdma_ctx.h"
#include "rdma_mem.h"
#include "rdma_ops.h"

#define DEFAULT_PORT "7471"

struct bulk_info
{
    uint64_t addr;
    uint32_t rkey;
    uint64_t len;
} __attribute__((packed));

static void unpack_bulk_info(const struct bulk_info *info, uint64_t *addr, uint32_t *rkey, uint64_t *len)
{
    if (addr)
        *addr = ntohll_u64(info->addr);
    if (rkey)
        *rkey = ntohl(info->rkey);
    if (len)
        *len = ntohll_u64(info->len);
}

static double elapsed_sec(const struct timespec *start, const struct timespec *end)
{
    double s = (double)(end->tv_sec - start->tv_sec);
    double ns = (double)(end->tv_nsec - start->tv_nsec) / 1e9;
    return s + ns;
}

static double now_sec(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    int err = 0;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server-ip> <port> [bytes] [chunk]\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    const char *port = argv[2];
    const char *size_str = (argc >= 4) ? argv[3] : "1G";
    const char *chunk_str = (argc >= 5) ? argv[4] : "4M";
    const char *log_env = getenv("RDMA_BULK_LOG");
    const char *csv_env = getenv("RDMA_BULK_CSV");
    int log_on = (log_env && *log_env) || (csv_env && *csv_env);
    FILE *csv = NULL;
    uint64_t total = parse_size_bytes(size_str);
    uint64_t chunk = parse_size_bytes(chunk_str);
    if (total == 0 || chunk == 0)
    {
        fprintf(stderr, "Invalid size/chunk\n");
        return 1;
    }

    rdma_ctx c = {0};
    if (cm_create_channel_and_id(&c))
    {
        err = 1;
        goto cleanup;
    }
    if (cm_client_resolve(&c, ip, port, NULL))
    {
        err = 1;
        goto cleanup;
    }
    if (build_pd_cq_qp(&c, IBV_QPT_RC, 256, 128, 128, 1))
    {
        err = 1;
        goto cleanup;
    }
    if (cm_client_connect_only(&c, 1, 1))
    {
        err = 1;
        goto cleanup;
    }

    struct rdma_conn_param connp = {0};
    if (cm_wait_connected(&c, &connp))
    {
        err = 1;
        goto cleanup;
    }

    struct bulk_info info = {0};
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

    uint64_t remote_len = 0;
    unpack_bulk_info(&info, &c.remote_addr, &c.remote_rkey, &remote_len);
    if (total > remote_len)
    {
        fprintf(stderr, "Requested %" PRIu64 " bytes, remote has %" PRIu64 "; capping\n", total, remote_len);
        total = remote_len;
    }

    if (alloc_and_reg(&c, &c.buf_tx, &c.mr_tx, (size_t)chunk, IBV_ACCESS_LOCAL_WRITE))
    {
        err = 1;
        goto cleanup;
    }
    memset(c.buf_tx, 0x5a, (size_t)chunk);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    const int max_outstanding = 64;
    const int signal_every = 16;
    int inflight = 0;
    uint64_t completed = 0;
    int current_batch = 0;
    int batch_sizes[1024];
    int batch_head = 0;
    int batch_tail = 0;
    uint64_t sent = 0;
    uint64_t wr_id = 1;
    double last_log = now_sec();
    double last_cqe = last_log;
    double start_log = last_log;
    int csv_rows = 0;
    if (csv_env && *csv_env)
    {
        csv = fopen(csv_env, "w");
        if (!csv)
        {
            LOG_ERR("failed to open CSV: %s", csv_env);
        }
        else
        {
            fprintf(csv, "time_s,sent_mib,inflight,completed,cqe_gap_s\n");
        }
    }
    while (sent < total)
    {
        uint64_t remaining = total - sent;
        uint64_t this_chunk = remaining < chunk ? remaining : chunk;
        current_batch++;
        int do_signal = (current_batch == signal_every) || (sent + this_chunk == total);
        if (post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr + sent, c.remote_rkey, (size_t)this_chunk, wr_id++,
                       do_signal))
        {
            err = 1;
            goto cleanup;
        }
        inflight++;
        if (do_signal)
        {
            batch_sizes[batch_tail] = current_batch;
            batch_tail = (batch_tail + 1) % (int)(sizeof(batch_sizes) / sizeof(batch_sizes[0]));
            current_batch = 0;
        }
        if (inflight >= max_outstanding)
        {
            struct ibv_wc wc;
            if (poll_one(c.cq, &wc))
            {
                err = 1;
                goto cleanup;
            }
            inflight -= batch_sizes[batch_head];
            completed += (uint64_t)batch_sizes[batch_head];
            batch_head = (batch_head + 1) % (int)(sizeof(batch_sizes) / sizeof(batch_sizes[0]));
            last_cqe = now_sec();
        }
        sent += this_chunk;

        if (log_on)
        {
            double now = now_sec();
            if (now - last_log >= 1.0)
            {
                double mib = (double)sent / (1024.0 * 1024.0);
                LOGF("DATA", "progress sent=%" PRIu64 "B (%.2f MiB) inflight=%d completed=%" PRIu64,
                     sent, mib, inflight, completed);
                if (now - last_cqe > 2.0)
                {
                    LOGF("DATA", "no CQE for %.1fs (likely retries/backoff)", now - last_cqe);
                }
                if (csv)
                {
                    fprintf(csv, "%.2f,%.3f,%d,%" PRIu64 ",%.2f\n",
                            now - start_log, mib, inflight, completed, now - last_cqe);
                    fflush(csv);
                    csv_rows++;
                }
                last_log = now;
            }
        }
    }
    while (batch_head != batch_tail)
    {
        struct ibv_wc wc;
        if (poll_one(c.cq, &wc))
        {
            err = 1;
            goto cleanup;
        }
        inflight -= batch_sizes[batch_head];
        completed += (uint64_t)batch_sizes[batch_head];
        batch_head = (batch_head + 1) % (int)(sizeof(batch_sizes) / sizeof(batch_sizes[0]));
        last_cqe = now_sec();
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = elapsed_sec(&t0, &t1);
    double mib = (double)sent / (1024.0 * 1024.0);
    printf("RDMA client wrote %" PRIu64 " bytes in %.3f s (%.2f MiB/s)\n", sent, secs, mib / secs);

    rdma_disconnect(c.id);

cleanup:
    if (csv)
    {
        if (csv_rows == 0)
        {
            double now = now_sec();
            fprintf(csv, "%.2f,%.3f,%d,%" PRIu64 ",%.2f\n",
                    now - start_log, mib, inflight, completed, now - last_cqe);
        }
        fclose(csv);
    }
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

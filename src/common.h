/**
 * File: common.h
 * Purpose: Common macros, logging helpers, and small utilities shared by
 * samples.
 *
 * Overview:
 * Defines LOG/CHK helpers, endian helpers (htonll/ntohll), simple dump
 * functions for CQE/SGE/WR, and small structs (e.g., remote_buf_info) used to
 * exchange rkey/addr via CM private_data.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally
 * verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.454918Z
 */

#pragma once
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

struct remote_buf_info
{
    uint64_t addr; // remote virtual address
    uint32_t rkey; // remote rkey
} __attribute__((packed));

static inline uint64_t htonll_u64(uint64_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return (((uint64_t)htonl(x & 0xFFFFFFFFULL)) << 32) | htonl((uint32_t)(x >> 32));
#else
    return x;
#endif
}
static inline uint64_t ntohll_u64(uint64_t x)
{
    return htonll_u64(x);
}

static inline struct remote_buf_info pack_remote_buf_info(uint64_t addr, uint32_t rkey)
{
    struct remote_buf_info info = {.addr = htonll_u64(addr), .rkey = htonl(rkey)};
    return info;
}

static inline void unpack_remote_buf_info(const struct remote_buf_info *info, uint64_t *addr, uint32_t *rkey)
{
    if (addr)
        *addr = ntohll_u64(info->addr);
    if (rkey)
        *rkey = ntohl(info->rkey);
}

#ifdef RDMA_VERBOSE
#define LOG(fmt, ...) fprintf(stderr, "[%s] " fmt "\n", __func__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if (0)                                                                                                         \
            fprintf(stderr, "[%s] " fmt "\n", __func__, ##__VA_ARGS__);                                                \
    } while (0)
#endif
#define CHECK(x, msg)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        if ((x))                                                                                                       \
        {                                                                                                              \
            perror(msg);                                                                                               \
            LOG("FAIL at %s:%d", __FILE__, __LINE__);                                                                  \
            _exit(1);                                                                                                  \
        }                                                                                                              \
    } while (0)

static inline const char *qp_state_str(enum ibv_qp_state s)
{
    switch (s)
    {
    case IBV_QPS_RESET:
        return "RESET";
    case IBV_QPS_INIT:
        return "INIT";
    case IBV_QPS_RTR:
        return "RTR";
    case IBV_QPS_RTS:
        return "RTS";
    case IBV_QPS_SQD:
        return "SQD";
    case IBV_QPS_SQE:
        return "SQE";
    case IBV_QPS_ERR:
        return "ERR";
    default:
        return "?";
    }
}

static inline void dump_qp(const struct ibv_qp *qp)
{
    struct ibv_qp_attr attr = {0};
    struct ibv_qp_init_attr init = {0};
    int mask = IBV_QP_STATE | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
               IBV_QP_MIN_RNR_TIMER | IBV_QP_SQ_PSN | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
               IBV_QP_MAX_QP_RD_ATOMIC;
    if (ibv_query_qp((struct ibv_qp *)qp, &attr, mask, &init) == 0)
    {
        LOG("QP dump: qpn=%u state=%s mtu=%d sq_psn=%u rq_psn=%u max_rd_atomic=%u "
            "max_dest_rd_atomic=%u retry=%u rnr_retry=%u timeout=%u",
            qp->qp_num, qp_state_str(attr.qp_state), attr.path_mtu, attr.sq_psn, attr.rq_psn, attr.max_rd_atomic,
            attr.max_dest_rd_atomic, attr.retry_cnt, attr.rnr_retry, attr.timeout);
        LOG("QP caps: max_send_wr=%u max_recv_wr=%u max_sge={s=%u,r=%u}", init.cap.max_send_wr, init.cap.max_recv_wr,
            init.cap.max_send_sge, init.cap.max_recv_sge);
    }
    else
    {
        LOG("ibv_query_qp failed");
    }
}
/* prototype */

static inline void dump_mr(const struct ibv_mr *mr, const char *name, void *addr, size_t len)
{
    LOG("MR %-6s: addr=%p len=%zu lkey=0x%x rkey=0x%x", name, addr, len, mr->lkey, mr->rkey);
}

static inline const char *wc_opcode_str(enum ibv_wc_opcode op)
{
    switch (op)
    {
    case IBV_WC_SEND:
        return "SEND";
    case IBV_WC_RDMA_WRITE:
        return "RDMA_WRITE";
    case IBV_WC_RDMA_READ:
        return "RDMA_READ";
    case IBV_WC_RECV:
        return "RECV";
    case IBV_WC_RECV_RDMA_WITH_IMM:
        return "RECV_RDMA_IMM";
    default:
        return "?";
    }
}
/* prototype */

static inline void dump_wc(const struct ibv_wc *wc)
{
    LOG("WC: wr_id=%lu status=%d opcode=%s byte_len=%u qp_num=%u", (unsigned long)wc->wr_id, wc->status,
        wc_opcode_str(wc->opcode), wc->byte_len, wc->qp_num);
}
/* prototype */

static inline void dump_sge(const struct ibv_sge *s, const char *who)
{
    LOG("SGE(%s): addr=%#llx len=%u lkey=0x%x", who, (unsigned long long)s->addr, s->length, s->lkey);
}

static inline void dump_wr_rdma(const struct ibv_send_wr *wr)
{
    const char *op = (wr->opcode == IBV_WR_RDMA_WRITE)            ? "RDMA_WRITE"
                     : (wr->opcode == IBV_WR_RDMA_READ)           ? "RDMA_READ"
                     : (wr->opcode == IBV_WR_RDMA_WRITE_WITH_IMM) ? "RDMA_WRITE_WITH_IMM"
                                                                  : "?";
    LOG("WR: wr_id=%lu opcode=%s signaled=%d num_sge=%d remote_addr=%#llx "
        "rkey=0x%x",
        (unsigned long)wr->wr_id, op, !!(wr->send_flags & IBV_SEND_SIGNALED), wr->num_sge,
        (unsigned long long)wr->wr.rdma.remote_addr, wr->wr.rdma.rkey);
}

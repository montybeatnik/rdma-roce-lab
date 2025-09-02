/**
 * File: rdma_mem.h
 * Purpose: Memory helpers: allocation/registration helpers and associated
 * structs.
 *
 * Overview:
 * Abstracts page-aligned allocation using posix_memalign and registers memory
 * with proper access flags (LOCAL_WRITE/REMOTE_WRITE/REMOTE_READ). Provides
 * helpers to free & deregister in the right order.
 *
 * Notes:
 *  - This file is part of an educational RDMA sample showing connection setup,
 *    memory registration, and basic one-sided operations (WRITE/READ) and
 *    WRITE_WITH_IMM (two-sided notification). Comments are intentionally
 * verbose.
 *  - The code targets librdmacm + libibverbs (RoCEv2/SoftRoCE friendly).
 *
 * Generated: 2025-09-02T09:13:19.455812Z
 */

#pragma once
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "rdma_ctx.h"
/* prototype */

int alloc_and_reg(rdma_ctx *c, void **buf, struct ibv_mr **mr, size_t len,
                  int access_flags);
/* prototype */
void mem_free_all(rdma_ctx *c);

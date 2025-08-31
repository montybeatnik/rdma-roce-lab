
#pragma once
#include <stdlib.h>
#include <string.h>
#include "rdma_ctx.h"
#include "common.h"

int alloc_and_reg(rdma_ctx *c, void **buf, struct ibv_mr **mr,
                  size_t len, int access_flags);
void mem_free_all(rdma_ctx *c);

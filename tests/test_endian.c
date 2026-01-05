#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/common.h"

int main(void)
{
    int err = 0;
    uint64_t x = 0x1122334455667788ULL;
    uint64_t nx = htonll_u64(x);
    uint64_t x2 = ntohll_u64(nx);
    if (x2 != x)
    {
        fprintf(stderr, "FAIL: x2 mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }

    struct remote_buf_info r = {.addr = htonll_u64(0xAABBCCDDEEFF0011ULL), .rkey = htonl(0x12345678)};
    if (sizeof(r) != (sizeof(uint64_t) + sizeof(uint32_t)))
    {
        fprintf(stderr, "FAIL: remote_buf_info size mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    uint64_t addr = ntohll_u64(r.addr);
    uint32_t rkey = ntohl(r.rkey);
    if (addr != 0xAABBCCDDEEFF0011ULL)
    {
        fprintf(stderr, "FAIL: addr mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    if (rkey != 0x12345678)
    {
        fprintf(stderr, "FAIL: rkey mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }

    struct remote_buf_info packed = pack_remote_buf_info(0x0102030405060708ULL, 0x90ABCDEF);
    uint64_t p_addr = 0;
    uint32_t p_rkey = 0;
    unpack_remote_buf_info(&packed, &p_addr, &p_rkey);
    if (p_addr != 0x0102030405060708ULL)
    {
        fprintf(stderr, "FAIL: packed addr mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    if (p_rkey != 0x90ABCDEF)
    {
        fprintf(stderr, "FAIL: packed rkey mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }

cleanup:
    if (!err)
        puts("OK test_endian");
    return err;
}

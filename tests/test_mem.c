#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_aligned(void *p, size_t align)
{
    return ((uintptr_t)p % align) == 0;
}

int main(void)
{
    int err = 0;
    void *p = NULL;
    int rc = posix_memalign(&p, 4096, 8192);
    if (rc != 0)
    {
        fprintf(stderr, "FAIL: posix_memalign rc=%d at %s:%d\n", rc, __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    if (!p)
    {
        fprintf(stderr, "FAIL: NULL pointer at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    if (!is_aligned(p, 4096))
    {
        fprintf(stderr, "FAIL: alignment mismatch at %s:%d\n", __FILE__, __LINE__);
        err = 1;
        goto cleanup;
    }
    memset(p, 0xAB, 8192);

cleanup:
    free(p);
    if (!err)
        puts("OK test_mem");
    return err;
}

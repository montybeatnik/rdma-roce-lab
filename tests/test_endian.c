#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../src/common.h"

#define T(expr)                                                          \
  do {                                                                   \
    if (!(expr)) {                                                       \
      fprintf(stderr, "FAIL: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
      return 1;                                                          \
    }                                                                    \
  } while (0)

int main(void) {
  uint64_t x = 0x1122334455667788ULL;
  uint64_t nx = htonll_u64(x);
  uint64_t x2 = ntohll_u64(nx);
  T(x2 == x);

  struct remote_buf_info r = {.addr = htonll_u64(0xAABBCCDDEEFF0011ULL),
                              .rkey = htonl(0x12345678)};
  T(sizeof(r) == (sizeof(uint64_t) + sizeof(uint32_t)));
  uint64_t addr = ntohll_u64(r.addr);
  uint32_t rkey = ntohl(r.rkey);
  T(addr == 0xAABBCCDDEEFF0011ULL);
  T(rkey == 0x12345678);

  struct remote_buf_info packed =
      pack_remote_buf_info(0x0102030405060708ULL, 0x90ABCDEF);
  uint64_t p_addr = 0;
  uint32_t p_rkey = 0;
  unpack_remote_buf_info(&packed, &p_addr, &p_rkey);
  T(p_addr == 0x0102030405060708ULL);
  T(p_rkey == 0x90ABCDEF);

  puts("OK test_endian");
  return 0;
}

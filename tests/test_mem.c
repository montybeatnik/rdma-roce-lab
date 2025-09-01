#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T(expr)                                                          \
  do {                                                                   \
    if (!(expr)) {                                                       \
      fprintf(stderr, "FAIL: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
      return 1;                                                          \
    }                                                                    \
  } while (0)
static int is_aligned(void *p, size_t align) {
  return ((uintptr_t)p % align) == 0;
}

int main(void) {
  void *p = NULL;
  int rc = posix_memalign(&p, 4096, 8192);
  T(rc == 0);
  T(p != NULL);
  T(is_aligned(p, 4096));
  memset(p, 0xAB, 8192);
  free(p);
  puts("OK test_mem");
  return 0;
}
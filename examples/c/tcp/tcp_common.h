#pragma once

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline uint64_t parse_size_bytes(const char *s) {
  if (!s || !*s) return 0;
  char *end = NULL;
  uint64_t val = strtoull(s, &end, 10);
  if (end && *end) {
    char suffix = (char)tolower((unsigned char)*end);
    if (suffix == 'k') val *= 1024ULL;
    else if (suffix == 'm') val *= 1024ULL * 1024ULL;
    else if (suffix == 'g') val *= 1024ULL * 1024ULL * 1024ULL;
    else {
      fprintf(stderr, "Unknown size suffix '%c'\n", *end);
      exit(1);
    }
  }
  return val;
}

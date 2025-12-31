/**
 * TCP bulk receiver for throughput comparison.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "tcp_common.h"

static double elapsed_sec(const struct timespec *start,
                          const struct timespec *end) {
  double s = (double)(end->tv_sec - start->tv_sec);
  double ns = (double)(end->tv_nsec - start->tv_nsec) / 1e9;
  return s + ns;
}

int main(int argc, char **argv) {
  const char *port_str = (argc >= 2) ? argv[1] : "9000";
  const char *size_str = (argc >= 3) ? argv[2] : "1G";
  uint64_t total = parse_size_bytes(size_str);
  if (total == 0) {
    fprintf(stderr, "Usage: %s <port> <bytes|K|M|G>\n", argv[0]);
    return 1;
  }

  int port = atoi(port_str);
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons((uint16_t)port);
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(fd, 1) < 0) {
    perror("listen");
    return 1;
  }

  printf("TCP server listening on port %d, expecting %llu bytes\n", port,
         (unsigned long long)total);
  int cfd = accept(fd, NULL, NULL);
  if (cfd < 0) {
    perror("accept");
    return 1;
  }

  const size_t buf_sz = 4 * 1024 * 1024;
  char *buf = malloc(buf_sz);
  if (!buf) {
    perror("malloc");
    return 1;
  }

  uint64_t received = 0;
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  while (received < total) {
    size_t want = buf_sz;
    if (total - received < want) want = (size_t)(total - received);
    ssize_t n = recv(cfd, buf, want, 0);
    if (n <= 0) {
      if (n < 0) perror("recv");
      break;
    }
    received += (uint64_t)n;
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);

  double secs = elapsed_sec(&t0, &t1);
  double mib = (double)received / (1024.0 * 1024.0);
  printf("TCP server received %llu bytes in %.3f s (%.2f MiB/s)\n",
         (unsigned long long)received, secs, mib / secs);

  close(cfd);
  close(fd);
  free(buf);
  return 0;
}

/**
 * TCP bulk sender for throughput comparison.
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

static double elapsed_sec(const struct timespec *start, const struct timespec *end)
{
    double s = (double)(end->tv_sec - start->tv_sec);
    double ns = (double)(end->tv_nsec - start->tv_nsec) / 1e9;
    return s + ns;
}

int main(int argc, char **argv)
{
    int err = 0;
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server-ip> <port> [bytes|K|M|G]\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    const char *port_str = argv[2];
    const char *size_str = (argc >= 4) ? argv[3] : "1G";
    uint64_t total = parse_size_bytes(size_str);
    if (total == 0)
    {
        fprintf(stderr, "Invalid size '%s'\n", size_str);
        return 1;
    }

    int port = atoi(port_str);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    char *buf = NULL;
    if (fd < 0)
    {
        perror("socket");
        return 1;
    }
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        perror("inet_pton");
        err = 1;
        goto cleanup;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        err = 1;
        goto cleanup;
    }

    const size_t buf_sz = 4 * 1024 * 1024;
    buf = malloc(buf_sz);
    if (!buf)
    {
        perror("malloc");
        err = 1;
        goto cleanup;
    }
    memset(buf, 0x5a, buf_sz);

    uint64_t sent = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (sent < total)
    {
        size_t chunk = buf_sz;
        if (total - sent < chunk)
            chunk = (size_t)(total - sent);
        ssize_t n = send(fd, buf, chunk, 0);
        if (n <= 0)
        {
            if (n < 0)
                perror("send");
            break;
        }
        sent += (uint64_t)n;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = elapsed_sec(&t0, &t1);
    double mib = (double)sent / (1024.0 * 1024.0);
    printf("TCP client sent %llu bytes in %.3f s (%.2f MiB/s)\n", (unsigned long long)sent, secs, mib / secs);

cleanup:
    free(buf);
    close(fd);
    return err;
}

#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "server" ]; then
  exec /usr/local/bin/rdma_min_server "${2:-7471}"
elif [ "${1:-}" = "client" ]; then
  if [ -z "${2:-}" ]; then
    echo "Usage: rdma_min_run client <SERVER_IP> [PORT]" >&2
    exit 1
  fi
  exec /usr/local/bin/rdma_min_client "$2" "${3:-7471}"
fi

exec sleep infinity

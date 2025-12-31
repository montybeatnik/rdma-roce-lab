#!/usr/bin/env bash
set -euo pipefail
SERVER_IP="${1:?Usage: $0 <SERVER_IP> <PORT>}"
PORT="${2:-7471}"
exec ./rdma_client "$SERVER_IP" "$PORT"

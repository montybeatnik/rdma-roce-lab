#!/usr/bin/env bash
set -euo pipefail
SERVER_IP="${1:?Usage: $0 <SERVER_IP> <PORT>}"
PORT="${2:-7472}"
exec ./rdma_client_imm "$SERVER_IP" "$PORT"

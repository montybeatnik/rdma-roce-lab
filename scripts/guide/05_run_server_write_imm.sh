#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-7472}"
exec ./rdma_server_imm "$PORT"

#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-7471}"
exec ./rdma_server "$PORT"

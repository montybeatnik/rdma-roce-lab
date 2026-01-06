#!/usr/bin/env bash
set -euo pipefail

SERVER_IP="${1:?Usage: $0 <SERVER_IP> <PORT> <TOTAL> <CHUNK> [CSV_PATH]}"
PORT="${2:-7471}"
TOTAL="${3:-256K}"
CHUNK="${4:-64K}"
CSV_PATH="${5:-/tmp/rdma_bulk.csv}"

RDMA_BULK_LOG=1 RDMA_BULK_CSV="$CSV_PATH" ./rdma_bulk_client "$SERVER_IP" "$PORT" "$TOTAL" "$CHUNK"

python3 scripts/guide/rdma_bulk_plot.py "$CSV_PATH" "/tmp/rdma_bulk.png"

echo "[INFO] CSV: $CSV_PATH"
echo "[INFO] Plot: /tmp/rdma_bulk.png"

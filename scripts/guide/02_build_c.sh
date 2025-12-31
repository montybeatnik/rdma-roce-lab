#!/usr/bin/env bash
set -euo pipefail

VM1="${VM1:-rdma-server}"
VM2="${VM2:-rdma-client}"
REPO="${REPO:-/home/ubuntu/rdma-roce-lab}"
TARGETS="${TARGETS:-all}"

if ! command -v multipass >/dev/null 2>&1; then
  echo "[ERR] multipass not found. Run scripts/guide/01_multipass_setup.sh first."
  exit 1
fi

for vm in "$VM1" "$VM2"; do
  echo "[INFO] Building on ${vm} (make ${TARGETS})"
  multipass exec "${vm}" -- bash -lc "cd ${REPO} && make ${TARGETS}"
done

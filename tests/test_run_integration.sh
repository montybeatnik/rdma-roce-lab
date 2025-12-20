#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Check verbs device presence
if ! ibv_devices | grep -E 'rxe|mlx|cx|hns|qedr|bnxt_re' >/dev/null 2>&1; then
  echo "[SKIP] No RDMA device detected by ibv_devices; skipping integration tests."
  exit 0
fi

make -s

PORT1=7471
PORT2=7472
SERVER_IP="${1:-}"
if [ -z "${SERVER_IP}" ]; then
  SERVER_IP="$(ip -o -4 addr show | awk '$2 != "lo" {print $4; exit}' | cut -d/ -f1)"
fi
[ -n "${SERVER_IP}" ] || SERVER_IP="127.0.0.1"

# Example 1: WRITE + READ
echo "[INFO] Starting server (example 1)…"
./rdma_server "$PORT1" > tests/server1.log 2>&1 &
SPID=$!
sleep 1

echo "[INFO] Running client (example 1)…"
set +e
./rdma_client "$SERVER_IP" "$PORT1" > tests/client1.log 2>&1
RC=$?
set -e

kill "$SPID" >/dev/null 2>&1 || true

grep -q "WRITE complete" tests/client1.log || { echo "[FAIL] Example 1: no WRITE complete"; exit 1; }
grep -q "READ complete"  tests/client1.log || { echo "[FAIL] Example 1: no READ complete";  exit 1; }
echo "[PASS] Example 1 (WRITE+READ)"

# Example 2: WRITE_WITH_IMM + RECV
echo "[INFO] Starting server (example 2)…"
./rdma_server_imm "$PORT2" > tests/server2.log 2>&1 &
SPID=$!
sleep 1

echo "[INFO] Running client (example 2)…"
set +e
./rdma_client_imm "$SERVER_IP" "$PORT2" > tests/client2.log 2>&1
RC=$?
set -e

kill "$SPID" >/dev/null 2>&1 || true

grep -q "WRITE_WITH_IMM completed" tests/client2.log || { echo "[FAIL] Example 2: no WRITE_WITH_IMM complete"; exit 1; }
grep -q "Got RECV with IMM"      tests/server2.log || { echo "[FAIL] Example 2: server missing IMM RECV";   exit 1; }
echo "[PASS] Example 2 (WRITE_WITH_IMM + RECV)"

#!/usr/bin/env bash
set -euo pipefail

LAB_NAME=${LAB_NAME:-frr-evpn-fabric}
SERVER=${SERVER:-gpu1}
CLIENT=${CLIENT:-gpu2}
PORT=${PORT:-7471}
SERVER_IP=${SERVER_IP:-10.10.10.101}
CLIENT_SRC_IP=${CLIENT_SRC_IP:-10.10.10.102}
SERVER_RXE=${SERVER_RXE:-rxe-gpu1}
CLIENT_RXE=${CLIENT_RXE:-rxe-gpu2}

server_ctr="clab-${LAB_NAME}-${SERVER}"
client_ctr="clab-${LAB_NAME}-${CLIENT}"

log() { printf "\n\033[1;34m[INFO]\033[0m %s\n" "$*"; }

ensure_rxe() {
  local ctr="$1"
  local rxe="$2"
  sudo docker exec "$ctr" sh -lc "rdma link delete \"$rxe\" 2>/dev/null || true; rdma link add \"$rxe\" type rxe netdev eth1 2>/dev/null || true"
}

log "Preparing RDMA device in ${server_ctr}"
ensure_rxe "$server_ctr" "$SERVER_RXE"
if ! sudo docker exec "$server_ctr" sh -lc "test -e /dev/infiniband/rdma_cm && ibv_devices | grep -q \"$SERVER_RXE\""; then
  echo "[WARN] RDMA device not visible in ${server_ctr}; skipping RDMA run." >&2
  exit 0
fi

log "Starting minimal RDMA server in ${server_ctr}"
sudo docker exec -d -e RDMA_BIND_IP="$SERVER_IP" "$server_ctr" sh -lc "rdma_min_run server \"$PORT\" >/tmp/rdma_server.log 2>&1 &"

log "Waiting for server to listen"
sleep 2
if sudo docker exec "$server_ctr" sh -lc "grep -q \"FAIL\" /tmp/rdma_server.log"; then
  echo "[WARN] Server failed to start. Server log:" >&2
  sudo docker exec "$server_ctr" sh -lc "tail -n +1 /tmp/rdma_server.log || true" >&2
  exit 1
fi

log "Preparing RDMA device in ${client_ctr}"
ensure_rxe "$client_ctr" "$CLIENT_RXE"
log "Running minimal RDMA client in ${client_ctr}"
attempt=1
while [ $attempt -le 3 ]; do
  set +e
  sudo docker exec -e RDMA_SRC_IP="$CLIENT_SRC_IP" "$client_ctr" timeout 8 rdma_min_run client "$SERVER_IP" "$PORT"
  rc=$?
  set -e
  if [ $rc -eq 0 ]; then
    exit 0
  fi
  if [ $rc -eq 124 ]; then
    echo "[WARN] Client attempt $attempt timed out; dumping server log..." >&2
    sudo docker exec "$server_ctr" sh -lc "tail -n +1 /tmp/rdma_server.log || true" >&2
  fi
  echo "[WARN] Client attempt $attempt failed; retrying..." >&2
  attempt=$((attempt + 1))
  sleep 2
done
echo "[WARN] Client failed after retries. Server log:" >&2
sudo docker exec "$server_ctr" sh -lc "tail -n +1 /tmp/rdma_server.log || true" >&2
exit 1

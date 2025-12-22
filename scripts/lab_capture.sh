#!/usr/bin/env bash
set -euo pipefail

CAPTURE_HOST="${CAPTURE_HOST:-rdma-server}"
CAPTURE_CLIENT_HOST="${CAPTURE_CLIENT_HOST:-rdma-client}"
CAPTURE_USER="${CAPTURE_USER:-ubuntu}"
CAPTURE_SECONDS="${CAPTURE_SECONDS:-10}"
CAPTURE_REMOTE="${CAPTURE_REMOTE:-${CAPTURE_REPO:-/home/ubuntu/rdma-roce-lab}/roce_capture.pcap}"
CAPTURE_LOCAL="${CAPTURE_LOCAL:-}"
CAPTURE_REPO="${CAPTURE_REPO:-/home/ubuntu/rdma-roce-lab}"
CAPTURE_PORT="${CAPTURE_PORT:-7471}"
CAPTURE_RDMA_DEV="${CAPTURE_RDMA_DEV:-rxe0}"
CAPTURE_SERVER_IP="${CAPTURE_SERVER_IP:-}"
CAPTURE_CLIENT_IP="${CAPTURE_CLIENT_IP:-}"
CAPTURE_BIND_IP="${CAPTURE_BIND_IP:-off}"
CAPTURE_CLIENT_SRC_IP="${CAPTURE_CLIENT_SRC_IP:-off}"
CAPTURE_INITIATOR_DEPTH="${CAPTURE_INITIATOR_DEPTH:-}"
CAPTURE_RESPONDER_RESOURCES="${CAPTURE_RESPONDER_RESOURCES:-}"
CAPTURE_USE_SCRIPTS="${CAPTURE_USE_SCRIPTS:-1}"
CAPTURE_SHOW_HOST_INFO="${CAPTURE_SHOW_HOST_INFO:-1}"
CAPTURE_RUN_APPS="${CAPTURE_RUN_APPS:-1}"
CAPTURE_BUILD="${CAPTURE_BUILD:-1}"
CAPTURE_FETCH_LOGS="${CAPTURE_FETCH_LOGS:-1}"
CAPTURE_LIVE="${CAPTURE_LIVE:-0}"
CAPTURE_WIRESHARK_BIN="${CAPTURE_WIRESHARK_BIN:-}"
CAPTURE_NETNS="${CAPTURE_NETNS:-}"
CAPTURE_IFACE="${CAPTURE_IFACE:-}"
CAPTURE_FILTER="${CAPTURE_FILTER:-udp port 4791 or ether proto 0x8915 or arp or icmp}"
CAPTURE_SERVER_WAIT_SECONDS="${CAPTURE_SERVER_WAIT_SECONDS:-3}"
CAPTURE_CLIENT_DELAY_SECONDS="${CAPTURE_CLIENT_DELAY_SECONDS:-0}"
CAPTURE_SERVER_RUN_SECONDS="${CAPTURE_SERVER_RUN_SECONDS:-}"
CAPTURE_CLIENT_RETRIES="${CAPTURE_CLIENT_RETRIES:-2}"
CAPTURE_CLIENT_RETRY_DELAY="${CAPTURE_CLIENT_RETRY_DELAY:-2}"

resolve_host() {
  local name="$1"
  local ip=""

  if command -v multipass >/dev/null 2>&1; then
    ip="$(multipass info "$name" 2>/dev/null | awk '/IPv4/ {print $2; exit}')"
  fi
  if [ -z "$ip" ] && command -v dscacheutil >/dev/null 2>&1; then
    ip="$(dscacheutil -q host -a name "$name" | awk '/ip_address/ {print $2; exit}')"
  elif [ -z "$ip" ] && command -v getent >/dev/null 2>&1; then
    ip="$(getent hosts "$name" | awk '{print $1; exit}')"
  fi

  if [ -n "$ip" ]; then
    echo "$ip"
  else
    echo "$name"
  fi
}

SERVER_SSH_HOST="$(resolve_host "$CAPTURE_HOST")"
CLIENT_SSH_HOST="$(resolve_host "$CAPTURE_CLIENT_HOST")"
if [ -z "$CAPTURE_IFACE" ]; then
  if [ -n "$CAPTURE_NETNS" ]; then
    CAPTURE_IFACE="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "sudo ip netns exec ${CAPTURE_NETNS} ip -o link show | awk -F\": \" '{print \$2}' | grep -v lo | head -n1")"
  else
    CAPTURE_IFACE="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "rdma link | awk -v dev='${CAPTURE_RDMA_DEV}' 'index(\$2, dev)==1 {for (i=1;i<=NF;i++) if (\$i==\"netdev\") {print \$(i+1); exit}}'")"
  fi
  if [ -z "$CAPTURE_IFACE" ]; then
    CAPTURE_IFACE="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "ip -o link show | awk -F\": \" '{print \$2}' | grep -E '^enp0s1$$' | head -n1")"
  fi
  if [ -z "$CAPTURE_IFACE" ]; then
    CAPTURE_IFACE="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "ip -o link show | awk -F\": \" '{print \$2}' | grep -v lo | head -n1")"
  fi
fi

if [ -n "$CAPTURE_SERVER_IP" ]; then
  SERVER_IP="$CAPTURE_SERVER_IP"
else
  if [ -n "$CAPTURE_NETNS" ]; then
    SERVER_IP="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "sudo ip netns exec ${CAPTURE_NETNS} ip -o -4 addr show dev ${CAPTURE_IFACE} | awk '{print \$4}' | cut -d/ -f1 | head -n1")"
  else
    SERVER_IP="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} \
      "ip -o -4 addr show dev ${CAPTURE_IFACE} | awk '{print \$4}' | cut -d/ -f1 | head -n1")"
  fi
  if [ -z "$SERVER_IP" ]; then
    SERVER_IP="$(ssh ${CAPTURE_USER}@${SERVER_SSH_HOST} "hostname -I | awk '{print \$1}'")"
  fi
fi

if [ -n "$CAPTURE_CLIENT_IP" ]; then
  CLIENT_IP="$CAPTURE_CLIENT_IP"
else
  if [ -n "$CAPTURE_NETNS" ]; then
    CLIENT_IP="$(ssh ${CAPTURE_USER}@${CLIENT_SSH_HOST} \
      "sudo ip netns exec ${CAPTURE_NETNS} ip -o -4 addr show dev ${CAPTURE_IFACE} | awk '{print \$4}' | cut -d/ -f1 | head -n1")"
  else
    CLIENT_IP="$(ssh ${CAPTURE_USER}@${CLIENT_SSH_HOST} \
      "ip -o -4 addr show dev ${CAPTURE_IFACE} | awk '{print \$4}' | cut -d/ -f1 | head -n1")"
  fi
  if [ -z "$CLIENT_IP" ]; then
    CLIENT_IP="$(ssh ${CAPTURE_USER}@${CLIENT_SSH_HOST} "hostname -I | awk '{print \$1}'")"
  fi
fi

if [ "$CAPTURE_BIND_IP" = "auto" ]; then
  SERVER_BIND_IP="$SERVER_IP"
elif [ -z "$CAPTURE_BIND_IP" ] || [ "$CAPTURE_BIND_IP" = "off" ]; then
  SERVER_BIND_IP=""
else
  SERVER_BIND_IP="$CAPTURE_BIND_IP"
fi

if [ "$CAPTURE_CLIENT_SRC_IP" = "auto" ]; then
  CLIENT_SRC_IP="$CLIENT_IP"
elif [ -z "$CAPTURE_CLIENT_SRC_IP" ] || [ "$CAPTURE_CLIENT_SRC_IP" = "off" ]; then
  CLIENT_SRC_IP=""
else
  CLIENT_SRC_IP="$CAPTURE_CLIENT_SRC_IP"
fi

fetch_logs() {
  [ "$CAPTURE_FETCH_LOGS" = "1" ] || return 0
  scp "${CAPTURE_USER}@${SERVER_SSH_HOST}:/tmp/rdma_server.log" ./rdma_server.log >/dev/null 2>&1 || true
  scp "${CAPTURE_USER}@${CLIENT_SSH_HOST}:/tmp/rdma_client.log" ./rdma_client.log >/dev/null 2>&1 || true
  scp "${CAPTURE_USER}@${SERVER_SSH_HOST}:/tmp/roce_capture.log" ./roce_capture.log >/dev/null 2>&1 || true
}

cleanup_remote() {
  ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" \
    "pkill -f rdma_server >/dev/null 2>&1 || true; rm -f /tmp/rdma_server.log /tmp/roce_capture.log" || true
  ssh "${CAPTURE_USER}@${CLIENT_SSH_HOST}" \
    "pkill -f rdma_client >/dev/null 2>&1 || true; rm -f /tmp/rdma_client.log" || true
}

if [ "$CAPTURE_BUILD" != "0" ]; then
  echo "[INFO] Building on VMs"
  ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" "bash -lc 'cd ${CAPTURE_REPO} && make -s'" >/dev/null
  ssh "${CAPTURE_USER}@${CLIENT_SSH_HOST}" "bash -lc 'cd ${CAPTURE_REPO} && make -s'" >/dev/null
fi

cleanup_remote

if [ "$CAPTURE_SHOW_HOST_INFO" = "1" ]; then
  echo "[INFO] Server host info"
  ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" "hostname; ip -o -4 addr show"
  echo "[INFO] Client host info"
  ssh "${CAPTURE_USER}@${CLIENT_SSH_HOST}" "hostname; ip -o -4 addr show"
fi

if [ "$CAPTURE_LIVE" = "1" ]; then
  if [ -z "$CAPTURE_WIRESHARK_BIN" ]; then
    if command -v wireshark >/dev/null 2>&1; then
      CAPTURE_WIRESHARK_BIN="$(command -v wireshark)"
    elif [ -x "/Applications/Wireshark.app/Contents/MacOS/Wireshark" ]; then
      CAPTURE_WIRESHARK_BIN="/Applications/Wireshark.app/Contents/MacOS/Wireshark"
    fi
  fi
  if [ -z "$CAPTURE_WIRESHARK_BIN" ]; then
    echo "[ERR] wireshark not found; set CAPTURE_WIRESHARK_BIN or install Wireshark" >&2
    exit 1
  fi
fi

capture_prefix_cmd() {
  if [ -n "$CAPTURE_NETNS" ]; then
    echo "sudo ip netns exec ${CAPTURE_NETNS} tcpdump"
  else
    echo "sudo tcpdump"
  fi
}

if [ "$CAPTURE_LIVE" = "1" ]; then
  echo "[INFO] Live capture on ${CAPTURE_HOST} for ${CAPTURE_SECONDS}s"
  echo "[INFO] Capture iface: ${CAPTURE_IFACE} (server IP ${SERVER_IP})"
  ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" \
    "sudo timeout ${CAPTURE_SECONDS} $(capture_prefix_cmd) -U -nni ${CAPTURE_IFACE} '${CAPTURE_FILTER}' -w -" \
    | "$CAPTURE_WIRESHARK_BIN" -k -i - &
  CAPTURE_PID=$!
else
  echo "[INFO] Capturing on ${CAPTURE_HOST} for ${CAPTURE_SECONDS}s"
  echo "[INFO] Capture iface: ${CAPTURE_IFACE} (server IP ${SERVER_IP})"
  ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" \
    "bash -lc 'mkdir -p \"$(dirname "${CAPTURE_REMOTE}")\" && sudo nohup timeout ${CAPTURE_SECONDS} $(capture_prefix_cmd) -i ${CAPTURE_IFACE} \"${CAPTURE_FILTER}\" -s 0 -w \"${CAPTURE_REMOTE}\" >/tmp/roce_capture.log 2>&1 &'"
fi

if [ "$CAPTURE_RUN_APPS" = "1" ]; then
  echo "[INFO] Starting server on ${CAPTURE_HOST}"
  SERVER_ENV=""
  if [ -n "$SERVER_BIND_IP" ]; then
    SERVER_ENV="RDMA_BIND_IP=${SERVER_BIND_IP} "
  fi
  if [ -n "$CAPTURE_INITIATOR_DEPTH" ]; then
    SERVER_ENV="${SERVER_ENV}RDMA_INITIATOR_DEPTH=${CAPTURE_INITIATOR_DEPTH} "
  fi
  if [ -n "$CAPTURE_RESPONDER_RESOURCES" ]; then
    SERVER_ENV="${SERVER_ENV}RDMA_RESPONDER_RESOURCES=${CAPTURE_RESPONDER_RESOURCES} "
  fi
  SERVER_RUN_SECONDS="${CAPTURE_SERVER_RUN_SECONDS:-$CAPTURE_SECONDS}"
  SERVER_CMD="./rdma_server"
  CLIENT_CMD="./rdma_client"
  if [ "$CAPTURE_USE_SCRIPTS" = "1" ]; then
    SERVER_CMD="./scripts/run_server.sh"
    CLIENT_CMD="./scripts/run_client.sh"
  fi
  if ! ssh "${CAPTURE_USER}@${SERVER_SSH_HOST}" \
    "bash -lc 'cd ${CAPTURE_REPO} && ${SERVER_ENV}nohup timeout ${SERVER_RUN_SECONDS} ${SERVER_CMD} ${CAPTURE_PORT} >/tmp/rdma_server.log 2>&1 &'"; then
    echo "[ERR] Server start failed" >&2
    fetch_logs
    exit 1
  fi
  sleep "${CAPTURE_SERVER_WAIT_SECONDS}"

  echo "[INFO] Running client on ${CAPTURE_CLIENT_HOST} (server ${SERVER_IP}:${CAPTURE_PORT})"
  sleep "${CAPTURE_CLIENT_DELAY_SECONDS}"
  CLIENT_ENV=""
  if [ -n "$CLIENT_SRC_IP" ]; then
    CLIENT_ENV="RDMA_SRC_IP=${CLIENT_SRC_IP} "
  fi
  if [ -n "$CAPTURE_INITIATOR_DEPTH" ]; then
    CLIENT_ENV="${CLIENT_ENV}RDMA_INITIATOR_DEPTH=${CAPTURE_INITIATOR_DEPTH} "
  fi
  if [ -n "$CAPTURE_RESPONDER_RESOURCES" ]; then
    CLIENT_ENV="${CLIENT_ENV}RDMA_RESPONDER_RESOURCES=${CAPTURE_RESPONDER_RESOURCES} "
  fi
  client_ok=0
  for attempt in $(seq 1 "${CAPTURE_CLIENT_RETRIES}"); do
    if ssh "${CAPTURE_USER}@${CLIENT_SSH_HOST}" \
      "bash -lc 'cd ${CAPTURE_REPO} && ${CLIENT_ENV}${CLIENT_CMD} ${SERVER_IP} ${CAPTURE_PORT} >/tmp/rdma_client.log 2>&1'"; then
      client_ok=1
      break
    fi
    echo "[WARN] Client run failed (attempt ${attempt}/${CAPTURE_CLIENT_RETRIES}); retrying..." >&2
    sleep "${CAPTURE_CLIENT_RETRY_DELAY}"
  done
  if [ "$client_ok" -ne 1 ]; then
    echo "[ERR] Client run failed" >&2
    fetch_logs
    exit 1
  fi
else
  echo "[INFO] CAPTURE_RUN_APPS=0; run server/client manually during capture window."
fi

sleep "${CAPTURE_SECONDS}"

if [ "$CAPTURE_LIVE" = "1" ]; then
  wait "$CAPTURE_PID" || true
else
  if [ -n "$CAPTURE_LOCAL" ]; then
    echo "[INFO] Fetching capture to ${CAPTURE_LOCAL}"
    scp "${CAPTURE_USER}@${SERVER_SSH_HOST}:${CAPTURE_REMOTE}" "${CAPTURE_LOCAL}"
  else
    echo "[INFO] Capture saved on VM at ${CAPTURE_REMOTE}"
  fi
fi
fetch_logs
echo "[INFO] Logs: ${CAPTURE_HOST}:/tmp/rdma_server.log ${CAPTURE_CLIENT_HOST}:/tmp/rdma_client.log"

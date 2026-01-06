#!/usr/bin/env bash
set -euo pipefail

IFACE=${IFACE:-$(ip -o route show default | awk '{print $5; exit}')}

if [[ -z "$IFACE" ]]; then
  echo "No default interface found. Set IFACE=eth0 (or similar)." >&2
  exit 1
fi

sudo tc qdisc del dev "$IFACE" root 2>/dev/null || true

echo "[INFO] netem cleared on ${IFACE}"
sudo tc qdisc show dev "$IFACE"

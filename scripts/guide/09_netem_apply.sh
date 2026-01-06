#!/usr/bin/env bash
set -euo pipefail

IFACE=${IFACE:-$(ip -o route show default | awk '{print $5; exit}')}
LOSS=${LOSS:-"1%"}
DELAY=${DELAY:-"0ms"}
JITTER=${JITTER:-"0ms"}
RATE=${RATE:-""}
REORDER=${REORDER:-"0%"}
DUPLICATE=${DUPLICATE:-"0%"}

if [[ -z "$IFACE" ]]; then
  echo "No default interface found. Set IFACE=eth0 (or similar)." >&2
  exit 1
fi

NETEM_OPTS="loss ${LOSS}"
if [[ "$DELAY" != "0ms" || "$JITTER" != "0ms" ]]; then
  if [[ "$JITTER" != "0ms" ]]; then
    NETEM_OPTS+=" delay ${DELAY} ${JITTER}"
  else
    NETEM_OPTS+=" delay ${DELAY}"
  fi
fi
if [[ -n "$RATE" ]]; then
  NETEM_OPTS+=" rate ${RATE}"
fi
if [[ "$REORDER" != "0%" ]]; then
  NETEM_OPTS+=" reorder ${REORDER}"
fi
if [[ "$DUPLICATE" != "0%" ]]; then
  NETEM_OPTS+=" duplicate ${DUPLICATE}"
fi

sudo tc qdisc replace dev "$IFACE" root netem ${NETEM_OPTS}

echo "[INFO] netem applied on ${IFACE}: ${NETEM_OPTS}"
sudo tc qdisc show dev "$IFACE"

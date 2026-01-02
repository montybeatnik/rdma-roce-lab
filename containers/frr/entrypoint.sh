#!/usr/bin/env bash
set -euo pipefail

mkdir -p /var/log/frr
touch /var/log/frr/frr.log
touch /var/log/frr/zebra.log
touch /var/log/frr/bgpd.log

if [ -f /etc/frr/daemons ]; then
  chown frr:frr /etc/frr/daemons || true
  chmod 640 /etc/frr/daemons || true
fi
if [ -f /etc/frr/frr.conf ]; then
  chown frr:frr /etc/frr/frr.conf || true
  chmod 640 /etc/frr/frr.conf || true
fi
chown -R frr:frr /var/log/frr || true

set +e
/usr/lib/frr/frrinit.sh start
RC=$?
set -e

if [ "$RC" -ne 0 ]; then
  echo "[WARN] frrinit failed with rc=$RC; keeping container alive for debug." >&2
fi

# Keep the container alive for containerlab exec commands
exec tail -f /var/log/frr/frr.log

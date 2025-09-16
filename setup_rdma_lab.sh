#!/usr/bin/env bash
set -euo pipefail

# -------- Settings --------
VM1=rdma-server
VM2=rdma-client
CPUS=4
MEM=4G
DISK=30G
UBU="jammy"                      # Ubuntu 22.04 ARM64
SSH_KEY="${HOME}/.ssh/id_ed25519.pub"   # change if you use id_rsa.pub

# -------- Helpers --------
need_cmd() { command -v "$1" >/dev/null 2>&1; }
log()  { printf "\n\033[1;34m[INFO]\033[0m %s\n" "$*"; }
warn() { printf "\n\033[1;33m[WARN]\033[0m %s\n" "$*"; }
die()  { printf "\n\033[1;31m[ERR]\033[0m %s\n" "$*"; exit 1; }

log "Starting RDMA lab setup"

# -------- Preflight --------
if [ ! -f "$SSH_KEY" ]; then
  warn "No SSH key found at $SSH_KEY"
  read -r -p "Generate a new ed25519 key now? [y/N] " yn || true
  if [[ "${yn:-N}" =~ ^[Yy]$ ]]; then
    ssh-keygen -t ed25519 -f "${HOME}/.ssh/id_ed25519" -N "" || die "Failed to generate SSH key"
    SSH_KEY="${HOME}/.ssh/id_ed25519.pub"
  else
    die "Please create an SSH key and re-run."
  fi
fi
PUBKEY=$(cat "$SSH_KEY")

need_cmd brew || die "Homebrew is required. Install from https://brew.sh and re-run."
if ! need_cmd multipass; then
  log "Installing Multipass via Homebrew"
  # If curl/HTTP2 is flaky on your network, install brewed curl then retry:
  # brew install curl && export PATH="/opt/homebrew/opt/curl/bin:$PATH" && export HOMEBREW_FORCE_BREWED_CURL=1
  brew install --cask multipass
fi

# -------- Cloud-init payload (portable heredoc) --------
CLOUDINIT="$(cat <<'EOF'
#cloud-config
package_update: true
package_upgrade: false
packages:
  - rdma-core
  - ibverbs-providers
  - ibverbs-utils
  - perftest
  - net-tools
  - tcpdump

write_files:
  - path: /usr/local/bin/setup_rxe.sh
    permissions: '0755'
    content: |
      #!/usr/bin/env bash
      set -euo pipefail
      IFACE="$(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1)"
      if [ -z "${IFACE:-}" ]; then echo "No NIC found" >&2; exit 1; fi
      modprobe rdma_rxe || true
      if ! rdma link | grep -q "netdev ${IFACE}"; then
        rdma link add rxe0 type rxe netdev "${IFACE}" || true
      fi
      rdma link || true
      ibv_devices || true
      ibv_devinfo || true

runcmd:
  - [ bash, -lc, "usermod -aG sudo ubuntu && echo 'ubuntu ALL=(ALL) NOPASSWD:ALL' >/etc/sudoers.d/99-ubuntu && chmod 440 /etc/sudoers.d/99-ubuntu" ]
  - [ bash, -lc, "mkdir -p ~ubuntu/.ssh && chmod 700 ~ubuntu/.ssh" ]
  - [ bash, -lc, "echo __PUBKEY__ >> ~ubuntu/.ssh/authorized_keys && chmod 600 ~ubuntu/.ssh/authorized_keys && chown -R ubuntu:ubuntu ~ubuntu/.ssh" ]
  - [ bash, -lc, "/usr/local/bin/setup_rxe.sh" ]
EOF
)"
# Inject your SSH key safely
CLOUDINIT="${CLOUDINIT/__PUBKEY__/$(printf '%s' "$PUBKEY" | sed 's/[&/\]/\\&/g')}"

# Write cloud-init to temp files (one per VM)
CI1=$(mktemp) ; CI2=$(mktemp)
printf "%s" "$CLOUDINIT" > "$CI1"
printf "%s" "$CLOUDINIT" > "$CI2"

# -------- Functions --------
launch_vm() {
  local name="$1" ci="$2"
  if multipass info "$name" >/dev/null 2>&1; then
    warn "VM $name already exists; leaving it as-is."
    return
  fi
  log "Launching VM $name (cpus=$CPUS, mem=$MEM, disk=$DISK)"
  multipass launch --name "$name" --cpus "$CPUS" --memory "$MEM" --disk "$DISK" --cloud-init "$ci" "$UBU"
}

# -------- Launch VMs --------
launch_vm "$VM1" "$CI1"
launch_vm "$VM2" "$CI2"

# -------- Retrieve IPs --------
IP1="$(multipass info "$VM1" 2>/dev/null | awk '/IPv4/ {print $2; exit}')"
IP2="$(multipass info "$VM2" 2>/dev/null | awk '/IPv4/ {print $2; exit}')"

# Wait briefly if IPs not yet assigned
for i in {1..20}; do
  [ -n "${IP1:-}" ] && [ -n "${IP2:-}" ] && break
  sleep 2
  IP1="$(multipass info "$VM1" 2>/dev/null | awk '/IPv4/ {print $2; exit}')"
  IP2="$(multipass info "$VM2" 2>/dev/null | awk '/IPv4/ {print $2; exit}')"
done

[ -n "${IP1:-}" ] || die "Could not get IP for $VM1"
[ -n "${IP2:-}" ] || die "Could not get IP for $VM2"

log "VMs are up:"
echo "  $VM1: $IP1"
echo "  $VM2: $IP2"

# -------- Connectivity & quick test --------
log "Testing SSH connectivity and verbs visibility"
ssh -o StrictHostKeyChecking=no ubuntu@"$IP1" "hostname && ibv_devices || true"
ssh -o StrictHostKeyChecking=no ubuntu@"$IP2" "hostname && ibv_devices || true"

log "Starting ib_write_bw server on $VM1"
ssh ubuntu@"$IP1" "nohup bash -lc 'ib_write_bw -d rxe0 -F -q 1 -t 1 >/tmp/ib_write_bw_server.log 2>&1 &'"

log "Running ib_write_bw client from $VM2 to $VM1 ($IP1)"
ssh ubuntu@"$IP2" "bash -lc 'sleep 1; ib_write_bw -d rxe0 -F $IP1 -q 1 -t 1'"

log "Server output:"
ssh ubuntu@"$IP1" "tail -n +1 /tmp/ib_write_bw_server.log || true"

cat <<EOF

 RDMA lab ready!

SSH:
  ssh ubuntu@$IP1
  ssh ubuntu@$IP2

Verify rxe:
  ssh ubuntu@$IP1 "rdma link; ibv_devices; ibv_devinfo -d rxe0"
  ssh ubuntu@$IP2 "rdma link; ibv_devices; ibv_devinfo -d rxe0"

Run perf tests:
  # on $VM1 (server):
  ib_write_bw -d rxe0 -F
  # on $VM2 (client):
  ib_write_bw -d rxe0 $IP1

Capture traffic:
  sudo tcpdump -i \$(ip -o link show | awk -F': ' '{print \$2}' | grep -v lo | head -n1) \\
       'udp port 4791 or ether proto 0x8915' -s 0 -w roce_capture.pcap

Teardown:
  multipass delete $VM1 $VM2 && multipass purge
EOF


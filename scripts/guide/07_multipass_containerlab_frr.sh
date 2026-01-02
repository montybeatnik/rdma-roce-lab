#!/usr/bin/env bash
set -euo pipefail

# -------- Settings --------
VM_NAME=${VM_NAME:-rdma-clab-frr}
CPUS=${CPUS:-4}
MEM=${MEM:-8G}
DISK=${DISK:-40G}
UBU=${UBU:-jammy}
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HOST_REPO="$(cd "${SCRIPT_DIR}/../.." && pwd)"
MOUNT_PATH="/home/ubuntu/rdma-roce-lab"

log()  { printf "\n\033[1;34m[INFO]\033[0m %s\n" "$*"; }
warn() { printf "\n\033[1;33m[WARN]\033[0m %s\n" "$*"; }

deploy_vm() {
  if multipass info "$VM_NAME" >/dev/null 2>&1; then
    warn "VM $VM_NAME already exists; leaving it as-is."
    return
  fi
  log "Launching $VM_NAME (cpus=$CPUS mem=$MEM disk=$DISK)"
  multipass launch --name "$VM_NAME" --cpus "$CPUS" --memory "$MEM" --disk "$DISK" "$UBU"
}

log "Creating Multipass VM for containerlab"
deploy_vm

log "Mounting repo"
multipass mount "$HOST_REPO" "$VM_NAME":"$MOUNT_PATH"

log "Installing docker + containerlab"
multipass exec "$VM_NAME" -- bash -lc "sudo apt update && sudo apt install -y docker.io iproute2 rdma-core ibverbs-utils && sudo usermod -aG docker ubuntu"
multipass exec "$VM_NAME" -- bash -lc "KVER=\$(uname -r); if apt-cache show linux-modules-extra-\$KVER >/dev/null 2>&1; then sudo apt install -y linux-modules-extra-\$KVER; else echo \"[WARN] linux-modules-extra-\$KVER not available\"; fi"
multipass exec "$VM_NAME" -- bash -lc "curl -sL https://get.containerlab.dev | sudo bash"

log "Building rdma-minimal container image"
multipass exec "$VM_NAME" -- bash -lc "cd $MOUNT_PATH && sudo docker build -t rdma-minimal:latest -f containers/minimal/Dockerfile ."

log "Building rdma-frr container image (arm64)"
multipass exec "$VM_NAME" -- bash -lc "cd $MOUNT_PATH && sudo docker build -t rdma-frr:latest -f containers/frr/Dockerfile ."

log "Setting CLAB_LABDIR_BASE for containerlab runs"
multipass exec "$VM_NAME" -- bash -lc "mkdir -p /home/ubuntu/.clab-runs && echo 'export CLAB_LABDIR_BASE=\"$HOME/.clab-runs\"' >> /home/ubuntu/.bashrc"

log "Enabling SoftRoCE in VM"
multipass exec "$VM_NAME" -- bash -lc "sudo modprobe rdma_rxe || echo '[WARN] rdma_rxe module missing; SoftRoCE not enabled'"
multipass exec "$VM_NAME" -- bash -lc "IF=\$(ip -o route show default | awk '{print \$5; exit}'); if [ -n \"\$IF\" ]; then sudo rdma link add rxe0 type rxe netdev \"\$IF\" 2>/dev/null || true; else echo '[WARN] No default route interface found; skipping rdma link add'; fi"

log "Deploying containerlab fabric (clean + reconfigure)"
multipass exec "$VM_NAME" -- bash -lc "cd $MOUNT_PATH/labs/containerlab-frr-evpn && export CLAB_LABDIR_BASE=\"$HOME/.clab-runs\" && sudo -E containerlab destroy -t lab.clab.yml || true"
multipass exec "$VM_NAME" -- bash -lc "ids=\$(sudo docker ps -aq --filter 'name=clab-frr-evpn-fabric'); if [ -n \"\$ids\" ]; then sudo docker rm -f \$ids; fi"
multipass exec "$VM_NAME" -- bash -lc 'LABDIR="'"$MOUNT_PATH"'/labs/containerlab-frr-evpn"; export CLAB_LABDIR_BASE="$HOME/.clab-runs"; if [ -e /dev/infiniband ]; then topo="$LABDIR/lab.clab.rdma.yml"; else topo="$LABDIR/lab.clab.yml"; fi; echo "[INFO] Using topology: $topo"; sudo -E containerlab deploy -t "$topo" --reconfigure'

if [ "${RUN_MINIMAL:-0}" = "1" ]; then
  log "Running minimal RDMA sample (if RDMA device is present)"
  multipass exec "$VM_NAME" -- bash -lc "if [ -e /dev/infiniband ]; then cd $MOUNT_PATH && bash scripts/guide/08_clab_run_minimal.sh || true; else echo '[INFO] /dev/infiniband not present; skipping RDMA run'; fi"
else
  log "Skipping minimal RDMA sample (set RUN_MINIMAL=1 to enable)"
fi

cat <<'EOF'

VM ready.

Next steps (inside the VM):
  multipass shell rdma-clab-frr
  cd /home/ubuntu/rdma-roce-lab/labs/containerlab-frr-evpn
  export CLAB_LABDIR_BASE="$HOME/.clab-runs"
  sudo -E containerlab deploy -t lab.clab.yml

To run the minimal RDMA sample separately:
  cd /home/ubuntu/rdma-roce-lab
  bash scripts/guide/08_clab_run_minimal.sh

To destroy:
  sudo containerlab destroy -t lab.clab.yml
EOF

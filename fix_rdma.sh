# If you see the following, run this script
#Did not detect devices
#If device exists, check if driver is up
#Unable to find the Infiniband/RoCE device



# --- Run this on your Mac terminal ---
for vm in rdma-client rdma-server; do
  echo "=== $vm ==="
  multipass exec "$vm" -- bash -lc '
    set -euo pipefail

    # 1) Ensure required packages (and the kernel extra modules containing rdma_rxe)
    sudo apt-get update -y
    sudo apt-get install -y rdma-core ibverbs-providers ibverbs-utils perftest net-tools tcpdump \
                            linux-modules-extra-$(uname -r)

    # 2) Load SoftRoCE kernel module
    sudo modprobe rdma_rxe

    # 3) Bind rxe to the primary NIC (figure out the default-route interface)
    IFACE=$(ip -o route show default | awk '"'"'{print $5; exit}'"'"')
    if [ -z "${IFACE:-}" ]; then
      echo "Could not determine primary interface" >&2
      ip -o link show
      exit 1
    fi

    # 4) (Re)create rxe0 on that NIC
    sudo rdma link del rxe0 2>/dev/null || true
    sudo rdma link add rxe0 type rxe netdev "$IFACE"

    echo "--- Status ---"
    echo "Interface: $IFACE"
    rdma link || true
    echo
    echo "/usr/lib/ibverbs.d:"
    ls -l /usr/lib/ibverbs.d || true
    echo
    echo "ibv_devices:"
    ibv_devices || true
  '
done


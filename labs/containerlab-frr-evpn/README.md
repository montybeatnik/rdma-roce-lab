# FRR EVPN/VXLAN Fabric (Containerlab)

This lab brings up a 2‑spine / 2‑leaf EVPN fabric using FRR containers and Linux VTEPs.
It is a conceptual EVPN/VXLAN model for lab use (not vendor‑exact).

## Topology
- spines: spine1, spine2 (RR for EVPN)
- leaves: leaf1, leaf2 (VTEPs, bridge `br10`, VNI 1010)
- hosts: gpu1, gpu2 (linux containers attached to leaf access ports)

## Deploy
From this directory:
```bash
sudo containerlab deploy -t lab.clab.yml
```

If you want RDMA inside containers and `/dev/infiniband` exists, use:
```bash
sudo containerlab deploy -t lab.clab.rdma.yml
```

If you are using the Multipass helper, it deploys the fabric only. The minimal RDMA sample is now a separate step:
```bash
make clab-evpn-deploy
```

To run the minimal RDMA sample via the helper after deploy:
```bash
multipass shell rdma-clab-frr
cd /home/ubuntu/rdma-roce-lab
bash scripts/guide/08_clab_run_minimal.sh
```

If you want the helper to run the minimal RDMA sample automatically:
```bash
RUN_MINIMAL=1 make clab-evpn-deploy
```

If you see ACL/permission errors on `clab-.../authorized_keys`, set a writable labdir:
```bash
mkdir -p ~/.clab-runs
export CLAB_LABDIR_BASE="$HOME/.clab-runs"
sudo -E containerlab deploy -t lab.clab.yml
```

If you see link/namespace errors (e.g., `/proc/.../ns/net`), clean up stale containers:
```bash
sudo containerlab destroy -t lab.clab.yml --cleanup || true
ids=$(sudo docker ps -aq --filter 'name=clab-frr-evpn-fabric')
if [ -n "$ids" ]; then sudo docker rm -f $ids; fi
sudo -E containerlab deploy -t lab.clab.yml --reconfigure
```

Inspect:
```bash
sudo containerlab inspect -t lab.clab.yml
```

Destroy:
```bash
sudo containerlab destroy -t lab.clab.yml
```

## Enter FRR (vtysh)
Use `vtysh` inside any leaf/spine container:
```bash
sudo docker exec -it clab-frr-evpn-fabric-leaf1 vtysh
```

Common FRR show commands:
```text
show ip bgp summary
show ip route 10.0.0.1
show ip route 10.0.0.2
show bgp l2vpn evpn summary
show bgp l2vpn evpn route
```

## Verify VXLAN/Bridge on leaves
```bash
sudo docker exec -it clab-frr-evpn-fabric-leaf1 sh -lc "ip -d link show vxlan10; bridge fdb show | head"
```

## Verify host connectivity (L2 over EVPN/VXLAN)
```bash
sudo docker exec -it clab-frr-evpn-fabric-gpu1 sh -lc "ping -c3 10.10.10.102"
```

## Run minimal RDMA samples in containers
Note: RDMA inside containers requires a host RDMA device. This lab uses SoftRoCE (rxe0) inside the VM host.

You can run the automated helper:
```bash
cd /home/ubuntu/rdma-roce-lab
bash scripts/guide/08_clab_run_minimal.sh
```

### Ensure RDMA device exists on the VM host
```bash
sudo modprobe rdma_rxe || true
IF=$(ip -o route show default | awk '{print $5; exit}')
sudo rdma link add rxe0 type rxe netdev "$IF" 2>/dev/null || true
rdma link
ibv_devices
```

You should see `rxe0` in `ibv_devices` and `/dev/infiniband` created:
```bash
ls -l /dev/infiniband
```

If `/dev/infiniband` is missing, use `lab.clab.yml` (non‑RDMA) and skip the steps below.

### Ensure RDMA devices are visible inside containers
Deploy the RDMA topology:
```bash
sudo -E containerlab deploy -t lab.clab.rdma.yml --reconfigure
```

Then verify inside each GPU container:
```bash
sudo docker exec -it clab-frr-evpn-fabric-gpu1 ibv_devices
sudo docker exec -it clab-frr-evpn-fabric-gpu2 ibv_devices
```

If `ibv_devices` is empty, create a SoftRoCE device inside the container netns:
```bash
sudo docker exec -it clab-frr-evpn-fabric-gpu1 rdma link add rxe-gpu1 type rxe netdev eth1 2>/dev/null || true
sudo docker exec -it clab-frr-evpn-fabric-gpu2 rdma link add rxe-gpu2 type rxe netdev eth1 2>/dev/null || true
sudo docker exec -it clab-frr-evpn-fabric-gpu1 ibv_devices
sudo docker exec -it clab-frr-evpn-fabric-gpu2 ibv_devices
```

Server (gpu1):
```bash
sudo docker exec -it -e RDMA_BIND_IP=10.10.10.101 clab-frr-evpn-fabric-gpu1 rdma_min_run server 7471
```

Client (gpu2):
```bash
sudo docker exec -it clab-frr-evpn-fabric-gpu2 rdma_min_run client 10.10.10.101 7471
```

If the client reports `RDMA_CM_EVENT_ADDR_ERROR`, force a source IP:
```bash
sudo docker exec -it -e RDMA_SRC_IP=10.10.10.102 clab-frr-evpn-fabric-gpu2 rdma_min_run client 10.10.10.101 7471
```

If it still fails, bind the server to the fabric IP and recreate rxe0 on eth1:
```bash
sudo docker exec -it clab-frr-evpn-fabric-gpu1 rdma link delete rxe-gpu1 2>/dev/null || true
sudo docker exec -it clab-frr-evpn-fabric-gpu1 rdma link add rxe-gpu1 type rxe netdev eth1 2>/dev/null || true
sudo docker exec -it -e RDMA_BIND_IP=10.10.10.101 clab-frr-evpn-fabric-gpu1 rdma_min_run server 7471
```

## Capture traffic on a spine (Wireshark/tcpdump)
On the VM host, capture VXLAN (UDP 4789) on the spine link. The interface name varies by host, so discover it first:
```bash
ip -o link | rg -n "spine1|leaf1|clab-frr-evpn-fabric"
```

Use the interface that corresponds to spine1's eth1 link (it may be a `veth*` name on the host). Example:
```bash
sudo tcpdump -nni <host_iface> udp port 4789 -w /tmp/evpn-vxlan.pcap
```

If you don't see a host interface for spine1, capture inside the spine container network namespace (requires `tcpdump` on the host):
```bash
PID=$(sudo docker inspect -f '{{.State.Pid}}' clab-frr-evpn-fabric-spine1)
sudo nsenter -t "$PID" -n tcpdump -nni eth1 udp port 4789 -w /tmp/evpn-vxlan.pcap
```

To map a container interface (e.g., spine1 `eth1`) to its host `veth` name, match `ifindex`/`peer_ifindex`:
```bash
PID=$(sudo docker inspect -f '{{.State.Pid}}' clab-frr-evpn-fabric-spine1)
sudo nsenter -t "$PID" -n ip -o link
```

Find the host veth whose `iflink` matches the container `eth1` ifindex:
```bash
IFINDEX=3  # replace with the eth1 index from the command above
for i in /sys/class/net/veth*/ifindex; do
  veth=$(basename "$(dirname "$i")")
  peer=$(cat "/sys/class/net/$veth/iflink")
  if [ "$peer" = "$IFINDEX" ]; then echo "$veth -> eth1"; fi
done
```

Then generate traffic (ping or RDMA). Stop the capture with Ctrl-C.

Copy the capture out of the VM:
```bash
multipass transfer rdma-clab-frr:/tmp/evpn-vxlan.pcap ./evpn-vxlan.pcap
```

Open with Wireshark:
```bash
wireshark ./evpn-vxlan.pcap
```

## Notes
- VTEPs are Linux `vxlan` interfaces on each leaf. FRR advertises EVPN routes; Linux handles data plane.
- If you want RDMA inside containers, bind `/dev/infiniband` and enable the RDMA stack in the VM/host.

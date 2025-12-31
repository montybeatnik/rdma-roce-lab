# Lab setup (Multipass + SoftRoCE)

These steps set up two Ubuntu 22.04 VMs with SoftRoCE and the RDMA toolchain.
Each script is short and readable so you can see what changes it makes and why.

## 1) Launch the lab
```bash
bash scripts/guide/01_multipass_setup.sh
```
Why: this creates two VMs, installs RDMA userspace tools, enables SoftRoCE, and
mounts the repo into `/home/ubuntu/rdma-roce-lab`.

## 2) Verify RDMA devices inside each VM
```bash
ibv_devices
```
You should see `rxe0` in the output.

## 3) Build the C examples on both VMs
```bash
bash scripts/guide/02_build_c.sh
```
Why: compiling inside the VM ensures the verbs libraries and headers match the
kernel modules and RDMA stack installed there.

## 4) Optional: install extra tools
```bash
sudo apt update
sudo apt install -y opensm infiniband-diags ibutils perftest mstflint
```

## 5) Capture RoCE traffic
```bash
sudo tcpdump -i $(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1) \
  'udp port 4791 or ether proto 0x8915' -s 0 -w roce_capture.pcap
```

## 6) Tear down
```bash
multipass delete rdma-client rdma-server && multipass purge
```

## Notes
- Multipass uses NAT by default. You can SSH from your host to each VM using the printed IPs.
- Inter-VM traffic crosses the virtual NICs, so tcpdump inside a VM sees RDMA CM (UDP/4791) and RoCEv2 data-plane (Ethertype 0x8915).

## Navigation
- Previous: [Architecture](architecture.md)
- Next: [Guided scripts](guide-scripts.md)

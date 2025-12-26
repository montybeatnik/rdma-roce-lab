# Lab notes (legacy)

For a clean, step-by-step guide, see `docs/lab-setup.md`.

## fire up lab
```bash
bash scripts/guide/01_multipass_setup.sh
```

## install gcc deps
```bash
sudo apt install build-essentialsudo apt update
sudo apt install opensm infiniband-diags ibutils rdma-core libibmad5 libibverbs-dev perftest mstflint -y
```

## Capture RDMA in one command
```bash
sudo tcpdump -i $(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1) \
  'udp port 4791 or ether proto 0x8915' -s 0 -w roce_capture.pcap
```

## What this gives you

Two Ubuntu 22.04 ARM64 VMs: rdma-a and rdma-b

SoftRoCE (rxe) automatically enabled and bound to the NIC in each VM

RDMA toolchain installed: rdma-core, ibverbs-*, perftest, tcpdump

Your Mac’s SSH key authorized in both VMs → ssh ubuntu@<ip> works out of the box

A quick bandwidth sanity test already run for you

Note: Multipass uses NAT by default. You can still SSH from your Mac to each VM using the printed IPs. Inter-VM traffic will traverse the virtual NICs—so tcpdump inside a VM will see both RDMA CM (UDP/4791) and RoCEv2 data-plane (Ethertype 0x8915).

## Tear down the lab 
```bash
multipass delete rdma-a rdma-b && multipass purge
```


## Errors 

```bash
curl: (92) HTTP/2 stream 1 was not closed cleanly: PROTOCOL_ERROR (err 1)

Error: Download failed on Cask 'multipass' with message: Download failed: https://github.com/canonical/multipass/releases/download/v1.16.1/multipass-1.16.1+mac-Darwin.pkg
```

```bash
rm -rf ~/Library/Caches/Homebrew/downloads/*
HOMEBREW_CURL_REUSE_CONNECTION=0 \
  /usr/bin/env curl --http1.1 -L -o ~/Downloads/multipass.pkg \
  https://github.com/canonical/multipass/releases/download/v1.16.1/multipass-1.16.1+mac-Darwin.pkg
sudo installer -pkg ~/Downloads/multipass.pkg -target /
```

### verification
```bash
multipass version
multipass launch --name test --cpus 2 --memory 2G --disk 10G jammy
multipass info test
multipass delete test && multipass purge
```


## RDMA read/writes 

### on both vms run the following
```bash
sudo apt update
sudo apt install -y build-essential rdma-core ibverbs-providers ibverbs-utils librdmacm-dev perftest
```

### run the server and the client code, respectively 
# VM1:
./rdma_server

# VM2 (use VM1’s IP and port 7471):
./rdma_client 10.0.2.15 7471



### compile the code 
#### server
gcc -O2 -std=c11 -Wall server.c -lrdmacm -libverbs -o rdma_server

#### client 
gcc -O2 -std=c11 -Wall client.c -lrdmacm -libverbs -o rdma_client

## Enable ECN 
### on both server/client
```
# Allow TCP ECN by default
sudo sysctl -w net.ipv4.tcp_ecn=1
sudo sysctl -w net.ipv4.tcp_ecn_fallback=0   # optional, strict mode

# Persist in sysctl.conf
echo "net.ipv4.tcp_ecn=1" | sudo tee -a /etc/sysctl.conf
```

### on server only 
```bash
# Install tc if missing
sudo apt-get install -y iproute2

# Replace the default queue with RED + ECN
sudo tc qdisc replace dev enp0s1 root red limit 100000 min 30000 max 35000 avpkt 1000 burst 20 probability 1.0 ecn
```

### confirm
```bash
tc -s qdisc show dev enp0s1
```

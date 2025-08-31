
# RDMA Modular Samples — One ZIP, Two Examples

Two minimal, modular C programs using **librdmacm** + **libibverbs**:

1) **Example 1**: One‑sided `RDMA_WRITE` (client→server) and `RDMA_READ` (client←server)  
2) **Example 2**: `RDMA_WRITE_WITH_IMM` (one‑sided data) + **server RECV notification** (two‑sided control)

Tested on Ubuntu 22.04 in Multipass with **SoftRoCE (rxe)**.

## Layout
```
.
├─ README.md
├─ Makefile
├─ scripts/
│  ├─ run_server.sh         # Example 1
│  ├─ run_client.sh         # Example 1
│  ├─ run_server_imm.sh     # Example 2
│  └─ run_client_imm.sh     # Example 2
└─ src/
   ├─ common.h
   ├─ rdma_ctx.h
   ├─ rdma_cm_helpers.{h,c}
   ├─ rdma_builders.{h,c}
   ├─ rdma_mem.{h,c}
   ├─ rdma_ops.{h,c}
   ├─ server_main.c         # Example 1 server
   ├─ client_main.c         # Example 1 client
   ├─ server_imm.c          # Example 2 server (WRITE_WITH_IMM + RECV)
   └─ client_imm.c          # Example 2 client
```

## Prereqs inside each VM
```bash
sudo apt update
sudo apt install -y build-essential librdmacm-dev rdma-core ibverbs-providers ibverbs-utils perftest \
                    linux-modules-extra-$(uname -r)

# SoftRoCE
sudo modprobe rdma_rxe
IF=$(ip -o route show default | awk '{print $5; exit}'); \
  sudo rdma link add rxe0 type rxe netdev "$IF" 2>/dev/null || true
ibv_devices   # should list rxe0
```

## Build
```bash
make
# or specific targets:
make rdma_server rdma_client rdma_server_imm rdma_client_imm
```

## Run — Example 1 (WRITE + READ)
On server VM:
```bash
./scripts/run_server.sh 7471
```
On client VM (use server IP):
```bash
./scripts/run_client.sh <SERVER_IP> 7471
```

## Run — Example 2 (WRITE_WITH_IMM + RECV notify)
On server VM:
```bash
./scripts/run_server_imm.sh 7472
```
On client VM:
```bash
./scripts/run_client_imm.sh <SERVER_IP> 7472
```

## Capture with tcpdump
```bash
sudo tcpdump -i $(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1) \
  'udp port 4791 or ether proto 0x8915' -s 0 -w roce_capture.pcap
```
Open in Wireshark; filter with `rdma_cm` and `rdmap || ddp`.

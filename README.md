
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

RDMA Lab — Architecture & Flow Overview

1. Repository Architecture

The repo is organized into shared modules, main executables, and tooling:
	•	Shared Interfaces / Headers
	•	common.h → Logging (LOG) and error checking (CHECK) macros.
	•	rdma_ops.h / rdma_mem.h → Prototypes for posting operations and memory helpers.
	•	Core Modules
	•	rdma_builders.c → Allocates and builds PDs, CQs, and QPs; provides dump utilities.
	•	rdma_mem.c → Allocates aligned buffers and registers MRs with ibv_reg_mr, exposing lkey/rkey.
	•	rdma_ops.c → Implements posting of RDMA READ, WRITE, SEND, RECV work requests.
	•	rdma_cm_helpers.c → Handles address/route resolution, connection setup, and CM event loop.
	•	Executables
	•	rdma_server → Standard server, exposes MR and accepts connections.
	•	rdma_client → Standard client, issues WRITE + READ requests.
	•	rdma_server_imm → Server variant that posts RECVs and handles immediate data.
	•	rdma_client_imm → Client variant that uses WRITE_WITH_IMM to deliver payloads + imm_data.
	•	Tooling
	•	scripts/run_server.sh / run_client.sh → Launch with env setup and logging.
	•	tests/… → Provides simple integration harnesses.
	•	Makefile → Build automation for all binaries.

External dependencies:
	•	libibverbs for verbs API (posting WRs, polling CQs).
	•	librdmacm for connection management (address resolution, QP state transitions).
	•	rxe (softRoCE) or RNIC drivers to emulate or provide RDMA transport.

⸻

2. Runtime System Flow

At runtime, the pieces come together into a clear flow:

Host A (Client)
	•	Starts rdma_client, builds PD, CQ, and QP using builders.
	•	Registers a local MR with ibv_reg_mr (gets lkey).
	•	Uses librdmacm to resolve address, exchange private_data (remote MR addr + rkey).
	•	Posts an RDMA_WRITE work request to its QP with local SGE (addr,len,lkey).
	•	RNIC doorbell signals the hardware to packetize and transmit.

Network
	•	Transport is RoCEv2: packets ride over UDP port 4791 with RDMAP/DDP headers.
	•	No kernel TCP/IP data path — NIC translates WQEs directly into wire protocol.

Host B (Server)
	•	Starts rdma_server, allocates its own PD, CQ, QP.
	•	Registers a buffer MR (gets rkey), which it shares via private_data.
	•	RNIC validates incoming packets against rkey and DMAs data directly into remote MR.
	•	Completion Queue (CQ) is updated with a CQE, which the application polls.

Result
	•	WRITE flow: client writes directly into server memory.
	•	READ flow: client posts RDMA_READ, NIC pulls from server MR using rkey, server is passive.
	•	IMM flow: client sends WRITE_WITH_IMM, server receives data + immediate value, with CQE.

⸻

3. Why It Matters
	•	Zero-copy: RNIC DMAs data directly between app buffers across hosts.
	•	Asynchronous: WRs are posted, completions signaled later.
	•	Security & isolation: PD + rkey ensure memory access only to explicitly shared regions.
	•	Versatility: Different executables show both one-sided (READ/WRITE) and two-sided (SEND/RECV with imm_data) semantics.

⸻

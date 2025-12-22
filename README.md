# RDMA RoCE Lab

Minimal, modular C samples using librdmacm + libibverbs for RDMA and RoCE learning.

Tested on Ubuntu 22.04 with SoftRoCE (rxe).

## Navigation
- Docs index: [docs/README.md](docs/README.md)

## Goals
- Keep each RDMA concept in a small, reusable module.
- Provide end-to-end examples for one-sided and immediate-data flows.
- Document how these flows map to real AI/ML systems.

## Learning path
1) Build and run the minimal sample in `examples/minimal/` to see the full flow without extra diagnostics.
2) Build and run Example 1 (WRITE + READ) to see one-sided operations in detail.
3) Build and run Example 2 (WRITE_WITH_IMM + RECV) to see notification flows.
4) Read `docs/architecture.md`, `docs/tutorial-narrative.md`, and `docs/ai-ml-use-cases.md`.
5) Use `examples/ai-ml/README.md` to extend the samples.

## Layout
```
.
├─ README.md
├─ Makefile
├─ docs/
├─ examples/
├─ scripts/
├─ src/
└─ tests/
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

## Run - Example 1 (WRITE + READ)
On server VM:
```bash
./scripts/run_server.sh 7471
```
On client VM (use server IP):
```bash
./scripts/run_client.sh <SERVER_IP> 7471
```

## Run - Example 2 (WRITE_WITH_IMM + RECV notify)
On server VM:
```bash
./scripts/run_server_imm.sh 7472
```
On client VM:
```bash
./scripts/run_client_imm.sh <SERVER_IP> 7472
```

## Docs
- [README](docs/README.md)
- [architecture](docs/architecture.md)
- [use-cases](docs/ai-ml-use-cases.md)
- [lab-setup](docs/lab-setup.md)
- [testing](docs/testing.md)
- [blog-post-ideas](docs/blog-post-ideas.md)
- [tutorial narrative](docs/tutorial-narrative.md)

## Examples
- `examples/ai-ml/README.md`
- `examples/minimal/README.md`
- `examples/rdma-bulk/README.md`
- `examples/tcp/README.md`

## Tests
```bash
make tests
# or
make test
```

## Capture with tcpdump
```bash
sudo tcpdump -i $(ip -o link show | awk -F': ' '{print $2}' | grep -v lo | head -n1) \
  'udp port 4791 or ether proto 0x8915' -s 0 -w roce_capture.pcap
```

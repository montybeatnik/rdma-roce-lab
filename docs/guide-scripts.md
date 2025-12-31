# Guided scripts (what and why)

This repo keeps the automation small and readable. Each script is meant to be a
teaching artifact, not a black box.

## 1) Build the lab with Multipass
`scripts/guide/01_multipass_setup.sh`
- **What it does**: creates two VMs, installs RDMA tooling, enables SoftRoCE,
  and mounts this repo into `/home/ubuntu/rdma-roce-lab`.
- **Why it exists**: we want a predictable lab with a known RDMA stack and a
  clean separation between client and server.

## 2) Create a Python venv
`scripts/guide/00_setup_venv.sh`
- **What it does**: creates `.venv` with system site packages and installs any
  Python deps from `requirements/py.txt`.
- **Why it exists**: `pyverbs` ships as a system package, but we still want a
  reproducible Python environment.

## 3) Compile the C examples inside the VMs
`scripts/guide/02_build_c.sh`
- **What it does**: runs `make` on both VMs.
- **Why it exists**: verbs headers and kernel modules need to match; compiling
  inside the VM avoids subtle ABI mismatches.

## 4) Run the basic WRITE + READ example
`scripts/guide/03_run_server_write_read.sh`  
`scripts/guide/04_run_client_write_read.sh`
- **What it does**: starts the server and client for the classic one-sided flow.
- **Why it exists**: this is the smallest complete data path and is ideal for
  step-by-step explanations.
- **Where to run**: inside each VM (or host) with the repo checked out.

## 5) Run the WRITE_WITH_IMM example
`scripts/guide/05_run_server_write_imm.sh`  
`scripts/guide/06_run_client_write_imm.sh`
- **What it does**: runs the immediate-data example to show a low-latency
  notification path.
- **Why it exists**: notification patterns are common in AI/ML pipelines.
- **Where to run**: inside each VM (or host) with the repo checked out.

## 6) Python minimal (RDMA CM)
Use Make targets for quick runs:
```bash
make py-list-devices
make py-query-ports PY_DEV=rxe0 PY_PORT=1
make py-minimal-server PY_CM_PORT=7471
make py-minimal-client PY_SERVER_IP=<SERVER_IP> PY_CM_PORT=7471
```

## Optional: RDMA vs TCP bulk comparison
Run the binaries directly:
```bash
./rdma_bulk_server 7471 1G
./rdma_bulk_client <SERVER_IP> 7471 1G 4M
./tcp_server 9000 1G
./tcp_client <SERVER_IP> 9000 1G
```

## Navigation
- Previous: [Lab setup](lab-setup.md)
- Next: [Tuning notes](tuning.md)

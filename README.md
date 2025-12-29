# RDMA RoCE Lab

Minimal, modular C and Python samples using `librdmacm` + `libibverbs` for RDMA/RoCE learning.
Tested on Ubuntu 22.04 with SoftRoCE (rxe).

## Hook
If your data plane is the nervous system of an AI or storage stack, RDMA is the
shortest path. This repo strips the path to essentials so you can see where
efficiency appears, where it leaks, and why.

## Why this lab exists
Modern AI fabrics treat the data center as the unit of compute, which means
every inefficiency gets multiplied. RDMA reduces CPU overhead and memory copies,
but the story does not end at peak throughput. Loss, retries, and stalled
queues turn “fast” into “wasteful” when scale and congestion show up. This lab
exists to make those tradeoffs visible: the deliberate setup costs, the fast
path that follows, and the points where retransmissions or backpressure can
erase the gains. It is a technical, hands-on way to connect protocol behavior
to resource use: CPU cycles, cache pollution, power draw, and the cooling and
water that follow. The goal is not moralizing. It is systems clarity. If you
can see where time and work are lost, you can design paths that shed less of
both.

## Stakes
RDMA reduces copies and CPU cycles, but it can hide cost in setup, signaling,
and congestion. The goal here is not just “fast,” but fewer wasted interrupts,
fewer round trips, and clearer intent in the data path.

## Mental Model
RDMA is a contracts-first workflow:
- **Memory is negotiated** (registered, keyed, and shared).
- **Queues are shared state** (QP state machines and CQ bookkeeping).
- **Completions are backpressure** (what you signal vs. what you ignore shapes throughput).

If you can map each transfer to these contracts, you can predict performance.

## 10-minute quickstart
Two VMs or two hosts on the same L2 network.

If you want a guided setup with Multipass on macOS:
```bash
bash scripts/guide/01_multipass_setup.sh
bash scripts/guide/02_build_c.sh
```

Prereqs inside each VM:
```bash
sudo apt update
sudo apt install -y build-essential librdmacm-dev rdma-core ibverbs-providers ibverbs-utils perftest \
  linux-modules-extra-$(uname -r) python3 python3-venv python3-pyverbs

sudo modprobe rdma_rxe
IF=$(ip -o route show default | awk '{print $5; exit}'); \
  sudo rdma link add rxe0 type rxe netdev "$IF" 2>/dev/null || true
ibv_devices
```

Build:
```bash
make
```

Python venv (optional):
```bash
bash scripts/guide/00_setup_venv.sh
source .venv/bin/activate
```

Python examples:
```bash
make py-list-devices
make py-query-ports PY_DEV=rxe0 PY_PORT=1
make py-minimal-server PY_CM_PORT=7471
make py-minimal-client PY_SERVER_IP=<SERVER_IP> PY_CM_PORT=7471
```

Run (WRITE + READ):
- Server VM:
```bash
./scripts/guide/03_run_server_write_read.sh 7471
```
- Client VM:
```bash
./scripts/guide/04_run_client_write_read.sh <SERVER_IP> 7471
```

## Labs (questions first)

### What happens in the minimal flow?
What you’ll learn: the end-to-end RDMA setup without extra diagnostics.  
What to try: `examples/c/minimal/README.md`  
What to observe: each step of address resolution, QP creation, and completion.

### How do one-sided writes and reads behave?
What you’ll learn: the difference between WRITE and READ and how they map to CQEs.  
What to try: `./scripts/guide/03_run_server_write_read.sh` and `./scripts/guide/04_run_client_write_read.sh`  
What to observe: ordering, CQ signaling, and where CPU time is spent.

### How does immediate data change the flow?
What you’ll learn: WRITE_WITH_IMM and how notifications change receiver behavior.  
What to try: `./scripts/guide/05_run_server_write_imm.sh` and `./scripts/guide/06_run_client_write_imm.sh`  
What to observe: how RECV posting pairs with immediate data.

### How does RDMA compare to TCP on bulk transfers?
What you’ll learn: where RDMA saves cycles and where it can stall.  
What to try: `examples/c/rdma-bulk/README.md` and `examples/c/tcp/README.md`  
What to observe: chunking, signaling, and throughput vs. CPU cost.

### How do these flows map to AI/ML systems?
What you’ll learn: how RDMA primitives appear in training and serving paths.  
What to try: `examples/c/ai-ml/README.md`  
What to observe: which RDMA constraints show up as “pipeline inefficiency.”

### What does the RDMA device look like from Python?
What you’ll learn: pyverbs device and port introspection.  
What to try: `examples/py/README.md`  
What to observe: device limits and port state before you build QPs.

## Invitation
Treat this repo as a lab notebook. If you see waste (extra copies, interrupts,
or retries), open an issue with measurements and notes. If something breaks,
it is usually the setup, not the verbs. (Usually.)

## What’s Next
- Read the docs index: `docs/README.md`
- Skim architecture and narrative: `docs/architecture.md`, `docs/tutorial-narrative.md`
- Follow the guided scripts: `docs/guide-scripts.md`
- Tune and choose verbs: `docs/tuning.md`, `docs/verbs-choices.md`
- Explore use-cases: `docs/ai-ml-use-cases.md`
- Run tests: `make tests` (C) and `make py-tests` (Python)
- Capture traffic: `scripts/lab_capture.sh`
- Python quick sample: `examples/py/README.md`

## TODO (optional)
- Add a scripted benchmark harness that runs RDMA and TCP side by side.

## Related posts
- [RDMA: the network powerhouse of an AI fabric (for now, anyway)](https://medium.com/@christopher_hern/rdma-the-network-powerhouse-of-an-ai-fabric-for-now-anyway-c50ce3f69879?source=your_stories_outbox---writer_outbox_published-----------------------------------------)
- TODO: The cost of moving data
- TODO: RDMA loss and recovery at scale

## Repo map
```
.
├─ README.md
├─ Makefile
├─ docs/
├─ examples/
│  ├─ c/
│  └─ py/
├─ scripts/
├─ src/
└─ tests/
```

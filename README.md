# RDMA RoCE Lab

Minimal, modular C samples using `librdmacm` + `libibverbs` for RDMA/RoCE learning.
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

Prereqs inside each VM:
```bash
sudo apt update
sudo apt install -y build-essential librdmacm-dev rdma-core ibverbs-providers ibverbs-utils perftest \
  linux-modules-extra-$(uname -r)

sudo modprobe rdma_rxe
IF=$(ip -o route show default | awk '{print $5; exit}'); \
  sudo rdma link add rxe0 type rxe netdev "$IF" 2>/dev/null || true
ibv_devices
```

Build:
```bash
make
```

Run (WRITE + READ):
- Server VM:
```bash
./scripts/run_server.sh 7471
```
- Client VM:
```bash
./scripts/run_client.sh <SERVER_IP> 7471
```

## Labs (questions first)

### What happens in the minimal flow?
What you’ll learn: the end-to-end RDMA setup without extra diagnostics.  
What to try: `examples/minimal/README.md`  
What to observe: each step of address resolution, QP creation, and completion.

### How do one-sided writes and reads behave?
What you’ll learn: the difference between WRITE and READ and how they map to CQEs.  
What to try: `./scripts/run_server.sh` and `./scripts/run_client.sh`  
What to observe: ordering, CQ signaling, and where CPU time is spent.

### How does immediate data change the flow?
What you’ll learn: WRITE_WITH_IMM and how notifications change receiver behavior.  
What to try: `./scripts/run_server_imm.sh` and `./scripts/run_client_imm.sh`  
What to observe: how RECV posting pairs with immediate data.

### How does RDMA compare to TCP on bulk transfers?
What you’ll learn: where RDMA saves cycles and where it can stall.  
What to try: `examples/rdma-bulk/README.md` and `examples/tcp/README.md`  
What to observe: chunking, signaling, and throughput vs. CPU cost.

### How do these flows map to AI/ML systems?
What you’ll learn: how RDMA primitives appear in training and serving paths.  
What to try: `examples/ai-ml/README.md`  
What to observe: which RDMA constraints show up as “pipeline inefficiency.”

## Invitation
Treat this repo as a lab notebook. If you see waste (extra copies, interrupts,
or retries), open an issue with measurements and notes.

## What’s Next
- Read the docs index: `docs/README.md`
- Skim architecture and narrative: `docs/architecture.md`, `docs/tutorial-narrative.md`
- Explore use-cases: `docs/ai-ml-use-cases.md`
- Run tests: `make tests`
- Capture traffic: `scripts/lab_capture.sh`

## TODO (optional)
- Add a scripted benchmark harness that runs RDMA and TCP side by side.

## Related posts
- [RDMA: the network powerhouse of an AI fabric (for now, anyway)](https://medium.com/@christopher_hern/rdma-the-network-powerhouse-of-an-ai-fabric-for-now-anyway-c50ce3f69879)
- TODO: The cost of moving data
- TODO: RDMA loss and recovery at scale

## Repo map
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

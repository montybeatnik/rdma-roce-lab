# RDMA RoCE Lab

Minimal, modular C and Python samples using `librdmacm` + `libibverbs` for RDMA/RoCE learning.  
Tested on Ubuntu 22.04 with SoftRoCE (rxe).

> If your data plane is the nervous system of an AI or storage stack, RDMA is the shortest path.  
> This repo strips the path to essentials so you can see where efficiency appears, where it leaks, and why.

## Start here (choose your path)

- **I want it working fast (10–15 min):** go to [Quickstart](#10-15-minute-quickstart-softroce)
- **I want the clean mental model:** read [Mental model](#mental-model), then run [Lab 1: Minimal flow](#lab-1-minimal-flow)
- **I want RDMA vs TCP behavior:** run [Lab 4: RDMA vs TCP](#lab-4-rdma-vs-tcp)
- **I learn visually first:** open [sims/README.md](sims/README.md) or jump to [Sims](#sims) for mechanics sims (QP states, verbs pipeline, MR keys/DMA, two-sided vs one-sided, CQ signaling)
- **I want Python (experimental):** see [Python status](#python-status-experimental)
- **I’m here because of the blog series:** see [Related posts](#related-posts)

## Why this lab exists

Modern AI fabrics treat the data center as the unit of compute — which means every inefficiency gets multiplied.  
RDMA reduces CPU overhead and memory copies, but the story doesn’t end at peak throughput.

Loss, retries, and stalled queues can turn “fast” into “wasteful” when scale and congestion show up.

This lab exists to make those tradeoffs visible:
- the deliberate setup costs,
- the fast path that follows,
- and the edge cases where backpressure, retransmissions, or signaling choices erase the gains.

The goal isn’t moralizing. It’s systems clarity:  
if you can see where time and work are lost, you can design paths that shed less of both.

## Mental model

RDMA is a **contracts-first workflow**:

- **Memory is negotiated** (registered, keyed, shared)
- **Queues are shared state** (QP state machines + CQ bookkeeping)
- **Completions are backpressure** (what you signal vs. ignore shapes throughput)

If you can map each transfer to these contracts, you can predict performance.

---

## 10–15 minute quickstart (SoftRoCE)

**Goal:** two hosts/VMs on the same L2 network, run a minimal WRITE + READ demo.

### Prereqs (run on BOTH hosts/VMs)

```bash
sudo apt update
sudo apt install -y build-essential librdmacm-dev rdma-core ibverbs-providers ibverbs-utils perftest \
  linux-modules-extra-$(uname -r)

sudo modprobe rdma_rxe

IF=$(ip -o route show default | awk '{print $5; exit}')
sudo rdma link add rxe0 type rxe netdev "$IF" 2>/dev/null || true

ibv_devices

# Expected: you should see rxe0 in the device list.

ubuntu@rdma-client:~/rdma-roce-lab$ ibv_devices
    device                 node GUID
    ------              ----------------
    rxe0                505400fffe78e0a6
```

### Get the repo + build (run on BOTH hosts/VMs)

```bash
git clone https://github.com/montybeatnik/rdma-roce-lab.git
cd rdma-roce-lab
make
```

### Run (WRITE + READ)

Server VM:

```bash
./scripts/guide/03_run_server_write_read.sh 7471
```

Client VM:

```bash
./scripts/guide/04_run_client_write_read.sh <SERVER_IP> 7471
```

Success check: the client log should include `RDMA_WRITE complete` and `RDMA_READ complete`.

### Optional: guided Multipass setup (macOS)

```bash
bash scripts/guide/01_multipass_setup.sh
bash scripts/guide/02_build_c.sh
```

### Troubleshooting (common misses)

- `rxe0` missing: install `linux-modules-extra-$(uname -r)` and reboot, then rerun `modprobe rdma_rxe`.
- `rdma link add` fails: confirm `rdma` is installed (`rdma-core`) and you picked a real NIC in `IF=...`.
- `ibv_devices` is empty: verify you are inside the VM and `rxe0` exists before running the examples.

## Python status (Experimental)

Python (pyverbs) is **experimental** in this repo. It may fail with tracebacks due to distro packaging, ABI mismatch, or `rdma-core` version differences. Use the C labs first, then try Python if you want the API shape.

See [docs/python-status.md](docs/python-status.md) for context and how to report issues. Python examples live in [examples/py/README.md](examples/py/README.md).

## Sims

HTML/Canvas sims you can open directly in a browser. Start with the ones below.

- `sims/average_vs_spikes.html` — why averages hide microbursts.
- `sims/microburst_queue_sim.html` — fan-in bursts filling a queue.
- `sims/amplification_feedback_loop.html` — how small delays compound into runaway.
- `sims/zero_copy_vs_copy.html` — TCP copy path vs RDMA zero-copy.
- `sims/rdma_slow_fast_path.html` — slow setup vs fast transfer path.
- `sims/cq_polling_vs_events.html` — CQ polling vs event-driven.
- `sims/loss_amplification_go_back_n.html` — retransmit amplification in a pipeline.
- `sims/topology_overlay.html` — leaf–spine vs rails under overlay pressure.
- `sims/topology_overlay_rail_aligned.html` — rail-aligned routing intuition.

See [sims/README.md](sims/README.md) for the full list and recording tips.

## Labs (questions first)

### Lab 1: Minimal flow

- **What you’ll learn:** the cleanest RDMA setup path without extra diagnostics.
- **Try:** `examples/c/minimal/README.md`
- **Observe:** address resolution → QP creation → posting → completions.

### Lab 2: One-sided ops (WRITE + READ)

- **What you’ll learn:** WRITE/READ behavior and how they map to CQEs.
- **Try:** `./scripts/guide/03_run_server_write_read.sh` + `./scripts/guide/04_run_client_write_read.sh`
- **Observe:** ordering, CQ signaling, where CPU time is spent.

### Lab 3: Notifications (WRITE_WITH_IMM)

- **What you’ll learn:** immediate data and receiver-side behavior.
- **Try:** `./scripts/guide/05_run_server_write_imm.sh` + `./scripts/guide/06_run_client_write_imm.sh`
- **Observe:** RECV posting, notification semantics, CQ behavior.

### Lab 4: RDMA vs TCP

- **What you’ll learn:** where RDMA saves cycles and where it can still stall.
- **Try:** `examples/c/rdma-bulk/README.md` and `examples/c/tcp/README.md`
- **Observe:** chunking, signaling, throughput vs CPU cost.

### Lab 5: AI/ML mapping

- **What you’ll learn:** how RDMA primitives map to training/serving paths.
- **Try:** `examples/c/ai-ml/README.md`
- **Observe:** which constraints show up as “pipeline inefficiency.”

### Lab 6: Python introspection (Experimental / optional)

- **What you’ll learn:** device + port introspection before you build QPs.
- **Try:** `examples/py/README.md` (may fail depending on your distro/rdma-core).
- **Observe:** limits, port state, device capabilities.

## Docs index

- `docs/architecture.md` — module boundaries, data flow, and lifecycle.
- `docs/guide-scripts.md` — what each guided script does and why.
- `docs/on-the-wire.md` — packet capture notes for the lab flows.
- `docs/verbs-choices.md` — which verbs map to which workloads.
- `docs/tuning.md` — performance knobs and their tradeoffs.
- `docs/testing.md` — unit and integration test guidance.
- `docs/ai-ml-use-cases.md` — practical AI/ML scenarios.
- `docs/python-status.md` — why pyverbs may fail and how to report it.

## Repo map

```
docs/       deeper explanations and references
examples/   C + Python samples
scripts/    guided scripts for setup and runs
sims/       HTML/Canvas visual sims
src/        shared C helpers
tests/      unit + integration checks
```

## Invitation

If you find a gap, a confusing step, or a better mental model, open an issue. This repo is small on purpose so the feedback loop stays tight.

## Related posts

- [RDMA: the network powerhouse of an AI fabric (for now, anyway)](https://medium.com/@christopher_hern/rdma-the-network-powerhouse-of-an-ai-fabric-for-now-anyway-c50ce3f69879?source=your_stories_outbox---writer_outbox_published-----------------------------------------)
- [The Cost of Moving Data](https://medium.com/@christopher_hern/the-cost-of-moving-data-28216bae734c)
- **TODO:** Post 3 

# Mechanics sims guide

This guide is a short, structured path through the mechanics-oriented ideas behind the sims.
It pairs each concept with a visual anchor so readers can build intuition before running code.

## 1) QP state machine (connection lifecycle)

**Goal:** Understand why state transitions exist and where a flow can stall.

Key ideas:
- QP moves through states (RESET → INIT → RTR → RTS → ERROR) in a strict order.
- CM events drive the transition to RTR/RTS for RC connections.
- A QP can be alive but unusable if it never reaches RTS.

Visual anchor:
- Think of the QP as a lock that only accepts work requests once the handshake is complete.

## 2) Verbs pipeline (objects and flow)

**Goal:** Know which objects are created and in which order for a minimal RC connection.

Typical order:
1) CM ID + event channel
2) Resolve address/route
3) PD + CQ + QP
4) Register MR
5) Connect/accept
6) Post send/recv
7) Poll CQ

Visual anchor:
- A pipeline of objects where each step depends on the last; missing one breaks the chain.

## 3) Memory registration (MR, lkey/rkey, DMA)

**Goal:** Understand why MR registration is a contract, not a formality.

Key ideas:
- MR pins memory and returns lkey/rkey; these are the permissions tokens.
- lkey is used by the local HCA; rkey is shared with the remote.
- Registration cost is part of the slow path, so reuse matters.

Visual anchor:
- Treat an MR like a capability ticket that allows DMA to touch memory safely.

## 4) Two-sided vs one-sided operations

**Goal:** See how RECV posting and CQ events differ between WRITE/READ and SEND/RECV.

Key ideas:
- One-sided (WRITE/READ) completes without the remote CPU touching data.
- Two-sided (SEND/RECV) requires the receiver to post a RECV and process completion.
- WRITE_WITH_IMM blurs the line: data is one-sided but still notifies the receiver.

Visual anchor:
- One-sided is “remote memory,” two-sided is “remote CPU.”

## 5) CQ depth and signaling

**Goal:** Understand how CQ size and signaling policy affect throughput and CPU.

Key ideas:
- Small CQ depth can overflow under bursts.
- Signaling every WR gives clear progress but adds overhead.
- Selective signaling reduces overhead but complicates flow control.

Visual anchor:
- CQ is a pressure gauge; signaling decides how often you read it.

## Suggested sim path

- `rdma_slow_fast_path.html` for setup vs data path cost
- `zero_copy_vs_copy.html` for data movement intuition
- `cq_polling_vs_events.html` for completion strategies

Then return to the C examples:
- `examples/c/minimal/README.md`
- `examples/c/write-read/README.md`
- `examples/c/write-imm/README.md`

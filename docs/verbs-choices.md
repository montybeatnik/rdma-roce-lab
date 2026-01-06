# Verbs choices (which operation fits which workload)

Think of verbs as a menu of tradeoffs. The right choice depends on how much
control you want and how much coordination you can afford.

## RDMA WRITE
- **Best for**: push-heavy workloads (gradients, embeddings, logs).
- **Why**: sender controls placement; receiver CPU stays off the data path.
- **Watch for**: receiver must expose and manage memory regions safely.

## RDMA READ
- **Best for**: pull-heavy workloads (parameter server, cache fills).
- **Why**: caller decides when to fetch; no receiver involvement on the fast path.
- **Watch for**: read latency can amplify tail latency under load.

## SEND/RECV
- **Best for**: message-style protocols or variable-sized payloads.
- **Why**: receiver controls buffer placement and can reject or reorder messages.
- **Watch for**: more CPU involvement, more synchronization.

## WRITE_WITH_IMM + RECV
- **Best for**: data plus a tiny notification (sequence number, watermark).
- **Why**: a single operation moves data and signals the receiver.
- **Watch for**: receiver still needs posted RECV buffers for the IMM path.

## Low-level mechanics (opcodes, WQEs, CQEs)

### Verbs opcodes (what you actually post)
When you call `ibv_post_send()` or `ibv_post_recv()`, you’re submitting a **work request (WR)** that includes an **opcode**:

- **Send queue opcodes**: `IBV_WR_RDMA_WRITE`, `IBV_WR_RDMA_READ`, `IBV_WR_SEND`, `IBV_WR_RDMA_WRITE_WITH_IMM`
- **Recv queue opcodes**: only `IBV_WR_RECV` (the opcode is implicit; you just post buffers)

These opcodes define **what the NIC does on the wire** and how the receiver observes completion.

### Completion opcodes (what you see in CQEs)
CQEs include an opcode that tells you **what completed**:

- `IBV_WC_RDMA_WRITE` / `IBV_WC_RDMA_READ` for one-sided ops
- `IBV_WC_RECV` for SEND/RECV
- `IBV_WC_RECV_RDMA_WITH_IMM` for WRITE_WITH_IMM

The opcode is how you infer **which path completed** and whether an IMM was delivered.

### Signaling and CQ load
WRs can be **signaled** (set `IBV_SEND_SIGNALED`) or not:
- Signaled WRs produce CQEs.
- Unsignaled WRs improve throughput but make progress tracking harder.

Common pattern: signal **every N** WRs and handle CQEs in batches.

### Sequence and retry (RC behavior)
RC uses **PSN (packet sequence numbers)** for ordering and reliability.
When a loss occurs:
- The receiver can request a retry.
- The sender retransmits from the first missing PSN (**Go-Back-N style**).

This is why small loss rates can create **large retransmit bursts**.

### RNR (Receiver Not Ready)
If the receiver has no posted RECV buffers:
- RC will signal **RNR NAK**
- The sender backs off and retries later (RNR retry count / timeout)

RNR is not data loss, but it **looks like loss** in terms of stalled progress.

### Immediate data
`WRITE_WITH_IMM` sends a 32-bit value in the packet:
- Delivered to the receiver CQE (`wc.imm_data`)
- Requires a posted RECV to observe

Use it for **sequence numbers, watermarks, or small tags**.

## RC vs UC vs UD (quick intuition)
- **RC (Reliable Connection)**: ordering + reliability, simplest mental model.
- **UC (Unreliable Connection)**: no retransmits, lower overhead, more risk.
- **UD (Unreliable Datagram)**: scalable, no connection state, smaller payloads.

When in doubt: start with RC, measure, then simplify.

## Navigation
- Previous: [Tuning notes](tuning.md)
- Next: [AI/ML use cases](ai-ml-use-cases.md)

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

## RC vs UC vs UD (quick intuition)
- **RC (Reliable Connection)**: ordering + reliability, simplest mental model.
- **UC (Unreliable Connection)**: no retransmits, lower overhead, more risk.
- **UD (Unreliable Datagram)**: scalable, no connection state, smaller payloads.

When in doubt: start with RC, measure, then simplify.

## Navigation
- Previous: [Tuning notes](tuning.md)
- Next: [AI/ML use cases](ai-ml-use-cases.md)

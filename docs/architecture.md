# Architecture

This repo is intentionally small but modular. Each source file owns a single concept and is reused by multiple examples.

## Modules
- src/common.h: logging, error helpers, endian helpers, and packing/unpacking of remote buffer info.
- src/rdma_ctx.h: shared context struct that wires CM, verbs objects, and buffers together.
- src/rdma_cm_helpers.c: address resolution, connection setup, and CM event handling.
- src/rdma_builders.c: create PD, CQ, and QP and dump QP state.
- src/rdma_mem.c: buffer allocation and MR registration/deregistration.
- src/rdma_ops.c: post RDMA WRITE/READ/RECV and poll CQ.
- src/server_main.c + src/client_main.c: Example 1 (RDMA WRITE + READ).
- src/server_imm.c + src/client_imm.c: Example 2 (WRITE_WITH_IMM + RECV notification).

## Data flow (Example 1: WRITE + READ)
1) Server listens, builds QP, registers a buffer MR, and sends {addr, rkey} in CM private_data.
2) Client resolves, builds QP, connects, receives {addr, rkey}.
3) Client posts RDMA_WRITE to the server MR and waits for a completion.
4) Client posts RDMA_READ from the server MR and waits for a completion.
5) Server observes its buffer contents changing without a receive path.

## Data flow (Example 2: WRITE_WITH_IMM)
1) Server posts a RECV to accept a notification and shares {addr, rkey}.
2) Client posts RDMA_WRITE_WITH_IMM to write data and deliver immediate data.
3) Server sees a RECV completion with imm_data and the updated buffer.

## Control plane vs data plane
- Control plane: rdma_cm handles address resolution, QP state transitions, and connection negotiation.
- Data plane: ibv_post_send/recv and ibv_poll_cq handle the RDMA work requests and completions.

## Extension points
- Add a new example by composing existing helpers with a new main file.
- Add a new module when the behavior is reused across examples (for example, message framing or buffer layout).

## Navigation
- Up: [Docs index](README.md)
- Next: [Lab setup](lab-setup.md)

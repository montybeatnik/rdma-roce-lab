# AI/ML examples (conceptual)

These examples are short design sketches that map the repo's RDMA building blocks to common AI/ML patterns.

## Example A: parameter server push/pull
- Client: treat `client_main.c` as a worker that WRITEs gradients and READs updated weights.
- Server: treat `server_main.c` as a parameter server exposing a weight buffer.
- Exercise: change the buffer contents to a fixed struct (layer_id, offset, payload).

## Example B: embedding cache invalidation
- Client: write an updated embedding vector with RDMA_WRITE.
- Client: send WRITE_WITH_IMM with the embedding id as `imm_data`.
- Server: use the RECV completion as the invalidation signal.

## Example C: inference batch queue
- Client: write a batch payload into a ring buffer with RDMA_WRITE.
- Client: send WRITE_WITH_IMM with batch sequence number.
- Server: pop the batch on RECV and hand to a local inference loop.

## Example D: feature ingestion watermark
- Client: stream data with RDMA_WRITE.
- Client: send WRITE_WITH_IMM with a watermark epoch to signal snapshot readiness.
- Server: on RECV, persist a snapshot and reset offsets.

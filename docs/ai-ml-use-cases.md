# AI/ML use cases for RDMA and RoCE

This repo focuses on low-level RDMA building blocks. The scenarios below show where those blocks map to real AI/ML systems.

## 1) Parameter server push/pull
- Pattern: workers push gradients (WRITE) and pull updated weights (READ).
- Mapping: Example 1 (client WRITE + READ) mirrors worker-to-parameter-server traffic.
- Why RDMA: low-latency weight updates without CPU copies.

## 2) Embedding cache service
- Pattern: query embeddings from a cache and update hot entries.
- Mapping: READ for lookup, WRITE for updates, and immediate data to signal cache invalidation.
- Why RDMA: faster tail latency for high-QPS recommendation or retrieval systems.

## 3) Feature store ingestion
- Pattern: streaming feature updates into a shared buffer with periodic snapshots.
- Mapping: WRITE for append-only buffers; immediate data for watermark/epoch signaling.
- Why RDMA: predictable ingestion latency under heavy load.

## 4) Distributed training rendezvous
- Pattern: workers publish readiness and small control messages before collectives.
- Mapping: WRITE_WITH_IMM for a small notification, RECV completion for the signal.
- Why RDMA: no kernel TCP/IP data path, low jitter.

## 5) Inference pipeline batching
- Pattern: producer writes batched requests into a shared ring buffer and signals the consumer.
- Mapping: WRITE for the batch payload, WRITE_WITH_IMM for a batch-id signal.
- Why RDMA: consistent latency and lower CPU overhead per request.

## 6) Training checkpoint staging
- Pattern: workers push checkpoint shards into a pinned buffer for background persistence.
- Mapping: RDMA_WRITE into a staging MR, optional READ to verify shard consistency.
- Why RDMA: keeps training loops hot while checkpoint IO proceeds asynchronously.

## 7) GPU-direct staging (conceptual)
- Pattern: stage data into pinned host buffers, then hand off to GPU DMA.
- Mapping: same RDMA writes as Example 1, but into pinned staging buffers.
- Why RDMA: higher throughput for data loaders and pre-processing pipelines.

## Suggested experiments
- Modify client_main.c to write a fixed-size record struct (simulate a gradient chunk).
- Add a sequence number to the immediate data path in client_imm.c and assert ordering in server_imm.c.
- Track write/read latency by timing before post_write/post_read and after poll_one.

## Navigation
- Previous: [Lab setup](lab-setup.md)
- Next: [Lab 7 netem](lab-7-netem.md)

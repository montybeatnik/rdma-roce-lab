# Tuning notes (what to tweak and why)

RDMA performance is mostly about avoiding unnecessary work. These are the knobs
you can turn in this repo, with the tradeoffs explained in plain language.

## Signal fewer completions
When every WR is signaled, the CPU spends more time polling CQEs than moving
data. Batch your completions instead.
- Where: `examples/c/rdma-bulk/rdma_bulk_client.c`
- Why: lower CQ pressure, fewer cache misses, less polling overhead.
- Risk: if you never signal, you lose visibility into progress.

## Increase outstanding WRs
Posting one WR and waiting for its CQE is a stop-and-wait protocol.
- Where: `examples/c/rdma-bulk/rdma_bulk_client.c`
- Why: pipelining keeps the NIC busy and hides round-trip latency.
- Risk: too many in-flight WRs can overflow the SQ or CQ.

## Tune chunk sizes
Small chunks add per-WQE overhead. Huge chunks can reduce fairness and amplify
loss impact.
- Where: `examples/c/rdma-bulk/rdma_bulk_client.c` (the `chunk` argument)
- Why: match chunk sizes to MTU, PCIe bandwidth, and CQ capacity.

## Reuse registered memory
Registering memory is expensive. Register once, reuse often.
- Where: `src/rdma_mem.c` and all sample apps
- Why: avoids repeated pinning and IOMMU work.

## Choose polling vs interrupts
Polling is fast but CPU-intensive; interrupts are efficient but can add jitter.
- Where: `src/rdma_ops.c` (`poll_one`)
- Why: predictable latency often beats lower CPU usage in AI/ML fabrics.

## Control initiator depth and responder resources
RDMA credits govern how many outstanding READ/WRITE operations are allowed.
- Where: `src/rdma_cm_helpers.c` via `RDMA_INITIATOR_DEPTH` and
  `RDMA_RESPONDER_RESOURCES`
- Why: deeper queues improve throughput but can increase head-of-line blocking.

## The meta rule
If you see wasted work (extra polls, extra CQEs, extra retries), measure it and
then remove it. The fastest data path is the one that does the least.

## Navigation
- Previous: [Guided scripts](guide-scripts.md)
- Next: [Verbs choices](verbs-choices.md)

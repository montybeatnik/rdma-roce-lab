# Mechanics sims

These sims focus on RDMA mechanics: states, objects, memory protection, and completions.

## Learning order

1) `qp_state_machine.html`
   - What you'll learn: QP states and the attributes that matter per transition.
   - Try this:
     - Step through RESET -> INIT -> RTR -> RTS and read the attributes panel.
     - Hit ERROR to see common failure causes.
   - Maps to repo code/docs: `docs/verbs-choices.md`, `docs/tutorial-narrative.md`

2) `rdma_verbs_pipeline.html`
   - What you'll learn: the object graph and the minimal RC connection flow.
   - Try this:
     - Autoplay to see the full handshake end-to-end.
     - Pause at post_recv and post_send to compare slow vs fast path.
   - Maps to repo code/docs: `examples/c/minimal/README.md`, `docs/architecture.md`

3) `mr_keys_dma.html`
   - What you'll learn: MR registration, lkey/rkey, and NIC DMA.
   - Try this:
     - Toggle remote access off to see protection errors.
     - Switch between RDMA_WRITE and RDMA_READ.
   - Maps to repo code/docs: `src/rdma_mem.c`, `docs/verbs-choices.md`

4) `two_sided_vs_one_sided.html`
   - What you'll learn: receiver involvement differences between SEND/RECV and WRITE/READ.
   - Try this:
     - Disable RECV and watch two-sided stall.
     - Change signaled ratio and watch CQE counts.
   - Maps to repo code/docs: `src/rdma_ops.c`, `docs/verbs-choices.md`

5) `cq_and_signaling.html`
   - What you'll learn: CQ depth, signaled WRs, and overflow risk.
   - Try this:
     - Increase send rate with low poll rate to force overflow.
     - Increase signal_every_n to reduce CQ pressure.
   - Maps to repo code/docs: `docs/tuning.md`, `docs/verbs-choices.md`

Back to sims index: `README.md`

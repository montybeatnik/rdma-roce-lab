# Simulations

These are single-file HTML/Canvas sims. Open any file in a browser (double-click or `open <file>`).

## Mechanics sims (states, verbs, MR/DMA, CQ)

See `MECHANICS.md` for the mechanics-focused sequence:
- QP state machine walkthrough
- Verbs pipeline (CM + objects + CQE)
- MR registration, lkey/rkey, DMA
- Two-sided vs one-sided operations
- CQ depth and signaling

## Learning order (start here)

1) `average_vs_spikes.html` — averages hide microbursts.
   - Try: spike probability 0.08, baseline 0.25, window 6s.
   - Try: spike probability 0.18, baseline 0.20, window 3s.

2) `microburst_queue_sim.html` — synchronized fan-in fills a queue.
   - Try: Synchronized, senders 12, arrival 1.2, drain 0.85, tight 0.12.
   - Try: Random mode with the same rates to see smoothing.

3) `amplification_feedback_loop.html` — small delays compound into runaway.
   - Try: Amplifying, speed 1.1, gain 1.25, noise 0.08.
   - Try: Damping, recovery 0.22 to show stabilization.

4) `zero_copy_vs_copy.html` — TCP copy path vs RDMA zero-copy.
   - Try: 64 KB, 500 msgs, copy cost 0.0010, interrupt 2.0.
   - Try: 256 KB, 200 msgs, poll cost 0.6 to see zero-copy win.

5) `rdma_slow_fast_path.html` — slow setup vs fast transfer path.
   - Try: reuse conn + MR, setup 1200, per-msg 6, count 200.
   - Try: reuse disabled to show lack of amortization.

6) `cq_polling_vs_events.html` — CQ polling vs event-driven.
   - Try: poll 6000, completion 1200, batch 8, wakeup 12.
   - Try: poll 1500, completion 1200, batch 16 (watch CPU drop).

7) `loss_amplification_go_back_n.html` — retransmit amplification.
   - Try: loss 1%, burst 2, inflight 16, coarse recovery.
   - Try: switch to selective and compare amplification.

8) `topology_overlay.html` — leaf–spine vs rails under overlay pressure.
   - Try: Compare preset, then click topology to show contrast.
   - Try: All-to-all + high hotspot to show shared links.

9) `topology_overlay_rail_aligned.html` — rail-aligned routing intuition.
   - Try: Fan-in with rails to show separation.
   - Try: Inc-AST to show staggered heat.

10) `incast_elephant_flows.html` — incast bursts vs elephant flow pressure.
   - Try: Incast, senders 16, arrival 1.3, drain 0.85, tightness 0.12.
   - Try: Staggered, elephant share 40% to show sustained queue pressure.

11) `nccl_ring_rail_alignment.html` — NCCL ring ordering vs rail crossings.
   - Try: Rail aligned with 16 GPUs (cross-rail drops).
   - Try: Worst case and note cross-rail hops = spine traversals.

12) `topology_tradeoffs_fat_tree.html` — why pick leaf-spine vs fat-tree vs rails.
   - Try: All-to-all and shuffle to compare hottest links.
   - Try: Fan-in + oversub 3:1 to show fragility.

## Notes

- Sims are deterministic enough for screen recording, but small random elements keep them alive.
- For Keynote or slides, record 10–20s clips and loop.

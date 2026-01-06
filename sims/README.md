# Simulations

These are single-file HTML/Canvas sims. Open any file in a browser (double-click or `open <file>`).

## Mechanics sims (states, verbs, MR/DMA, CQ)

See `MECHANICS.md` for the mechanics-focused sequence:
- QP state machine walkthrough
- Verbs pipeline (CM + objects + CQE)
- MR registration, lkey/rkey, DMA
- Two-sided vs one-sided operations
- CQ depth and signaling

Open: [MECHANICS.md](MECHANICS.md)

## Learning order (start here)

0) Lab-focused sims:
   - `lab1_minimal_flow.html` — minimal CM/QP/WR flow.
   - `lab2_one_sided_ops.html` — WRITE/READ and CQ signaling.
   - `lab3_write_with_imm.html` — notification flow with imm_data.
   - `lab4_rdma_vs_tcp.html` — bulk transfer cost comparison.
   - `lab5_ai_ml_mapping.html` — RDMA verbs mapped to AI/ML patterns.

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

## Notes

- Sims are deterministic enough for comparisons, but small random elements keep them alive.

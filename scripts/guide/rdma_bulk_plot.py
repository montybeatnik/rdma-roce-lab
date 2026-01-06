#!/usr/bin/env python3
import csv
import sys
from pathlib import Path

import matplotlib.pyplot as plt

def main():
    if len(sys.argv) < 3:
        print("Usage: rdma_bulk_plot.py <csv_path> <png_path>", file=sys.stderr)
        return 1

    csv_path = Path(sys.argv[1])
    png_path = Path(sys.argv[2])

    rows = []
    with csv_path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)

    if not rows:
        print("CSV is empty", file=sys.stderr)
        return 1

    t = [float(r["time_s"]) for r in rows]
    sent_mib = [float(r["sent_mib"]) for r in rows]
    inflight = [int(r["inflight"]) for r in rows]
    cqe_gap = [float(r["cqe_gap_s"]) for r in rows]

    fig, ax = plt.subplots(2, 1, figsize=(8, 6), sharex=True)

    ax[0].plot(t, sent_mib, color="#4da3ff", label="Sent (MiB)")
    ax[0].set_ylabel("MiB sent")
    ax[0].grid(True, alpha=0.2)
    ax[0].legend(loc="upper left")

    ax[1].plot(t, inflight, color="#ffb020", label="Inflight")
    ax[1].plot(t, cqe_gap, color="#ff5c5c", label="CQE gap (s)")
    ax[1].set_xlabel("Time (s)")
    ax[1].set_ylabel("Inflight / gap")
    ax[1].grid(True, alpha=0.2)
    ax[1].legend(loc="upper left")

    fig.tight_layout()
    fig.savefig(png_path, dpi=160)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

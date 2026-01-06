# Lab 7: Loss & Reordering with tc/netem

This lab introduces controlled loss, delay, jitter, reordering, and duplication using Linux `tc netem`.
Use it to visualize Go-Back-N retransmit amplification and the impact on throughput/CPU.

## Prereqs (inside each VM)

```bash
sudo apt update
sudo apt install -y iproute2 python3 python3-matplotlib
```

## Apply netem on a VM

Pick which VM you want to impair (client is a good default). Then run:

```bash
# Example: 1% loss + 40ms delay + 5ms jitter
LOSS=1% DELAY=40ms JITTER=5ms \
  scripts/guide/09_netem_apply.sh
```

Optional knobs:
- `LOSS=1%` (packet loss)
- `DELAY=40ms` (base delay)
- `JITTER=5ms` (variation around delay)
- `RATE=200mbit` (bandwidth cap)
- `REORDER=5%` (packet reordering)
- `DUPLICATE=0.5%` (duplicate packets)
- `IFACE=eth0` (override interface selection)

## Run a lab under impairment

Minimal (Lab 1/2):
```bash
# Server VM
./scripts/guide/03_run_server_write_read.sh 7471

# Client VM (with netem applied)
./scripts/guide/04_run_client_write_read.sh <SERVER_IP> 7471
```

Bulk comparison (Lab 4):
```bash
# RDMA server VM
./rdma_bulk_server 7471 256K

# RDMA client VM (with netem applied)
scripts/guide/11_rdma_bulk_report.sh <SERVER_IP> 7471 256K 64K
```

## Visual companion

Open `sims/loss_amplification_go_back_n.html` to see how loss amplification grows when you introduce loss or reordering.

## Clear netem

```bash
scripts/guide/10_netem_clear.sh
```

## Notes

- `netem` affects **all traffic** on the selected interface (RDMA CM + data plane), so expect broader impact.
- If you don’t see effects, double‑check `IFACE` is the active fabric interface.

## Navigation

- Previous: [AI/ML use cases](ai-ml-use-cases.md)
- Next: [Testing](testing.md)

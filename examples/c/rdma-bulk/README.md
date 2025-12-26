# RDMA bulk write (comparison baseline)

This example sends a large payload with RDMA WRITE in fixed-size chunks so you
can compare its throughput and capture to the TCP version.

## Build
From the repo root:
```bash
make rdma_bulk_server rdma_bulk_client
```

## Run
On server VM:
```bash
./rdma_bulk_server 7471 1G
```
On client VM (use server IP):
```bash
./rdma_bulk_client <SERVER_IP> 7471 1G 4M
```

Both sides print elapsed time and MiB/s.

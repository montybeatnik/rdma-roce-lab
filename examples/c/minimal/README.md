# Minimal RDMA sample (tutorial-friendly)

This folder contains the smallest runnable client/server pair in the repo. It is
designed for blog walk-throughs where you want to show the full RDMA flow without
the extra diagnostics used in the main samples.

## What it demonstrates
- RDMA CM as the control plane: resolve, connect, exchange a tiny blob of metadata.
- One-sided data plane: RDMA WRITE then RDMA READ to the same remote buffer.
- Why the metadata matters: the client needs the remote address and rkey to do
  one-sided operations safely.

## Build
From the repo root:
```bash
make minimal
```

## Run
On server VM:
```bash
./rdma_min_server 7471
```
On client VM (use server IP):
```bash
./rdma_min_client <SERVER_IP> 7471
```

## Where to look in code
- Server: `examples/c/minimal/server_min.c`
- Client: `examples/c/minimal/client_min.c`

## Real-world anchor: parameter server
Treat the server buffer as a "weights" region. The client WRITEs gradients and
READs back updated weights. The flow is identical; only the payload changes.

## Wireshark captures
- [on-the-wire](/docs/on-the-wire.md)
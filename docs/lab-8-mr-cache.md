# Lab 8: MR cache (user‑mode registration)

This lab demonstrates a simple **memory registration cache** keyed by size.
It models a common user‑mode pattern in AI/ML pipelines: reuse registered
buffers to avoid repeated `ibv_reg_mr` overhead.

## Build (inside each VM)

```bash
make mr_cache
```

## Run

Server VM:
```bash
./mr_cache_server 7473
```

Client VM (use server IP):
```bash
./mr_cache_client <SERVER_IP> 7473 40
```

## What to watch

The client logs MR cache stats at the end:
- `hits`: reused registrations
- `misses`: new registrations
- `regs`: total `ibv_reg_mr` calls

Higher hit rates show **effective reuse** and less registration overhead.

## How it works

- Client cycles through fixed sizes (4K, 8K, 16K, 32K).
- For each size, it requests an MR from the cache.
- If available, reuse it; otherwise, allocate + `ibv_reg_mr`.
- After the WRITE completes, it returns the MR to the cache.

## Where to look in code

- Client: `examples/c/mr-cache/client_mr_cache.c`
- Server: `examples/c/mr-cache/server_mr_cache.c`

## Navigation

- Previous: [Lab 7 netem](lab-7-netem.md)
- Next: [Testing](testing.md)

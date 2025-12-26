# TCP bulk transfer (comparison baseline)

This example sends a large payload over TCP so you can compare its capture and
throughput to the RDMA bulk example.

## Build
From the repo root:
```bash
make tcp_server tcp_client
```

## Run
On server VM:
```bash
./tcp_server 9000 1G
```
On client VM (use server IP):
```bash
./tcp_client <SERVER_IP> 9000 1G
```

Both sides print elapsed time and MiB/s.

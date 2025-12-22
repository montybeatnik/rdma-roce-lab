# RDMA on Ethernet: a practical walkthrough from setup to wire capture

This post serves as a tutorial for network engineers who want a deep, packet-
level understanding of RDMA, while still being readable to ML engineers and
general readers. We will:

1. Build a minimal RDMA flow (WRITE + READ).
2. Explain why the protocol needs a capability (addr + rkey).
3. Capture packets on the wire and identify the control and data planes.
4. Map the flow to AI/ML training and inference workloads.

## 1) RDMA in one sentence (and why that matters)
RDMA lets a client read/write memory on a remote host without involving the
remote CPU in the data path.

There are two critical underlying requirements:
- The remote host must explicitly register the memory.
- The client must present a capability (address + rkey) to access it.

Those requirements are why the setup and the code look the way they do.

## 2) Lab setup (what we need and why)
We use two VMs with SoftRoCE so RDMA can run on a normal Ethernet NIC.

- **Two VMs**: mirrors real client/server separation.
- **SoftRoCE (`rdma_rxe`)**: provides RDMA semantics on Ethernet.
- **`librdmacm`**: control plane (resolve, connect, exchange metadata).
- **`libibverbs`**: data plane (WRITE/READ work requests, CQ completions).

Setup script: `setup_rdma_lab.sh`

## 3) The minimal flow (15 lines that explain the whole protocol)
Use the minimal example in `examples/minimal/` to keep the narrative tight.

### Server: register memory and share the capability
```c
alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, BUF_SZ,
              IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                  IBV_ACCESS_REMOTE_WRITE);

struct remote_buf_info info =
    pack_remote_buf_info((uintptr_t)c.buf_remote, c.mr_remote->rkey);
cm_server_accept_with_priv(&c, &info, sizeof(info));
```
Why this exists: RDMA requires explicit memory registration and a capability
token (rkey) so the NIC can enforce access.

Full context: `examples/minimal/server_min.c`

### Client: use the capability to WRITE and READ
```c
unpack_remote_buf_info(&info, &c.remote_addr, &c.remote_rkey);

post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
           strlen((char *)c.buf_tx) + 1, 1, 1);
post_read(c.qp, c.mr_rx, c.buf_rx, c.remote_addr, c.remote_rkey, BUF_SZ, 2, 1);
```
Why this exists: the remote CPU is not on the data path; the capability grants
the NIC permission to perform the transfer.

Full context: `examples/minimal/client_min.c`

## 4) Packet-level view (what’s on the wire)
We can capture both the control plane and the data plane.

- **RDMA CM**: UDP port 4791 (connection handshake).
- **RoCEv2 data**: EtherType 0x8915 (RDMA write/read payloads).

![virtual_address_and_rkey](./images/va_rkey.png)

### Seen in the logs:
```bash
ubuntu@rdma-client:~/rdma-roce-lab$ ./scripts/run_client.sh 192.168.2.24 7471 
[main] Create CM channel + ID + connect
[cm_client_resolve] cm: getaddrinfo(192.168.2.24:7471)
[cm_client_resolve] cm: rdma_resolve_addr(timeout=5000ms)
[cm_client_resolve] cm: waiting ADDR_RESOLVED
[cm_client_resolve] cm: rdma_resolve_route(timeout=5000ms)
[cm_client_resolve] cm: waiting ROUTE_RESOLVED
[main] Build PD/CQ/QP
[dump_qp] QP dump: qpn=23 state=INIT mtu=1 sq_psn=0 rq_psn=0 max_rd_atomic=0 max_dest_rd_atomic=0 retry=0 rnr_retry=0 timeout=0
[dump_qp] QP caps: max_send_wr=63 max_recv_wr=63 max_sge={s=1,r=1}
[cm_client_connect_only] cm: rdma_connect()
[cm_client_connect_only] cm: rdma_connect() returned
[main] Got remote addr=0xaaab26b58000 rkey=0x220
[main] Register local tx/rx
[main] Post RDMA_WRITE
[dump_sge] SGE(WRITE): addr=0xaaaafb06a000 len=18 lkey=0x13f8
[dump_wr_rdma] WR: wr_id=1 opcode=RDMA_WRITE signaled=1 num_sge=1 remote_addr=0xaaab26b58000 rkey=0x220
[dump_wc] WC: wr_id=1 status=0 opcode=RDMA_WRITE byte_len=18 qp_num=23
[main] WRITE complete
[main] Post RDMA_READ
[dump_sge] SGE(READ): addr=0xaaaafb06c000 len=4096 lkey=0x1459
[dump_wr_rdma] WR: wr_id=2 opcode=RDMA_READ signaled=1 num_sge=1 remote_addr=0xaaab26b58000 rkey=0x220
[dump_wc] WC: wr_id=2 status=0 opcode=RDMA_READ byte_len=4096 qp_num=23
[main] READ complete: 'client-wrote-this'
[main] Done
```

Run capture:
```bash
make lab-capture-live CAPTURE_SECONDS=20 CAPTURE_SERVER_WAIT_SECONDS=5 # or
make lab-capture-live CAPTURE_SERVER_IP=192.168.2.24 CAPTURE_CLIENT_IP=192.168.2.25
make lab-capture-live CAPTURE_BIND_IP=off CAPTURE_CLIENT_SRC_IP=off CAPTURE_SECONDS=20 CAPTURE_SERVER_WAIT_SECONDS=5
```

```
# display filter for CM 
infiniband.deth.srcqp == 0x00000001
```

Figure placeholder: Wireshark capture overview (UDP/4791 + Ethertype 0x8915)
[FIGURE: wireshark_overview.png]
[PLACEHOLDER NOTE: insert screenshot of UDP/4791 CM packets + RoCEv2 frames]

Figure placeholder: CM connect request/response detail
[FIGURE: wireshark_cm_handshake.png]
[PLACEHOLDER NOTE: highlight private_data length and CM events]

Figure placeholder: RDMA WRITE payload frame
[FIGURE: wireshark_rdma_write.png]
[PLACEHOLDER NOTE: show RDMA WRITE opcode + payload bytes]

## 5) Mapping to real workloads (why protocol design matters)

### Parameter server (training)
- **WRITE**: workers push gradients into a shared buffer.
- **READ**: workers pull updated weights back.
- **Why RDMA**: no kernel copies, low tail latency at sync points.

Callout (client side):
```c
CHECK(post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                 payload_len, 1, 1),
      "post_write");
CHECK(post_read(c.qp, c.mr_rx, c.buf_rx, c.remote_addr, c.remote_rkey,
                BUF_SZ, 2, 1),
      "post_read");
```
Full context: `src/client_main.c`

### Training rendezvous
- **WRITE_WITH_IMM**: send a small "ready" signal alongside data.
- **Why RDMA**: immediate data is a low-overhead notification path.

Callout (client side):
```c
uint32_t imm = htonl(1); // "ready" flag for rendezvous
CHECK(post_write_with_imm(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                          payload_len, 2, imm),
      "post_write_with_imm");
```
Full context: `src/client_imm.c`

### Inference batching
- **WRITE**: producer writes the batch payload.
- **WRITE_WITH_IMM**: producer signals the batch id.
- **Why RDMA**: stable latency and low CPU overhead.

Callout (client side):
```c
CHECK(post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                 batch_bytes, 1, 1),
      "post_write");
uint32_t imm = htonl(batch_id);
CHECK(post_write_with_imm(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                          0, 2, imm),
      "post_write_with_imm");
```
Full context: `src/client_imm.c`

## 6) Operational constraints (what the protocol demands)
- **Memory registration cost**: pinning pages is not free.
- **Capability safety**: `rkey` limits access to a specific buffer.
- **Backpressure**: CQ polling is explicit; design your producer/consumer flow.

## 7) RDMA vs TCP (1 GB transfer comparison)
This section uses a 1 GB payload and compares:
- **RDMA bulk write**: one-sided WRITE in large chunks.
- **TCP bulk send**: classic send/recv stream.

```bash
make rdma_bulk_server rdma_bulk_client tcp_server tcp_client
```

### Run the RDMA bulk transfer
On server VM:
```bash
./rdma_bulk_server 7471 1G
```
On client VM:
```bash
./rdma_bulk_client <SERVER_IP> 7471 1G 4M
```

### Run the TCP transfer
On server VM:
```bash
./tcp_server 9000 1G
```
On client VM:
```bash
./tcp_client <SERVER_IP> 9000 1G
```

Both paths print elapsed seconds and MiB/s so you can compare timing directly.

### Capture and Wireshark analysis
Capture both runs and load the pcaps into Wireshark. Use these filters:
- RDMA data plane: `eth.type == 0x8915`
- RDMA CM control plane: `udp.port == 4791`
- TCP stream: `tcp.port == 9000`

Capture checklist (manual):
1) Start a capture on the server VM interface (`enp0s1`).
2) Run the RDMA bulk transfer.
3) Stop capture and save as `rdma_1g.pcap`.
4) Start a new capture.
5) Run the TCP transfer.
6) Stop capture and save as `tcp_1g.pcap`.

Wireshark IO Graph setup:
1) Open `rdma_1g.pcap`.
2) Go to `Statistics → IO Graphs`.
3) Set Y axis to `Bits/s`, Interval to `100 ms`.
4) Add a graph with display filter `eth.type == 0x8915`.
5) Repeat for `tcp_1g.pcap` with filter `tcp.port == 9000`.

Suggested Wireshark views:
- **Statistics → IO Graphs**: set Y axis to `Bits/s`, add one graph per filter.
- **Statistics → Conversations**: compare throughput for the TCP stream and the
  RDMA flow (RoCEv2 frames).
- **Statistics → Protocol Hierarchy**: confirm RDMA vs TCP share of bytes.

Figure placeholder: RDMA IO graph (1 GB)
[FIGURE: rdma_io_graph.png]
[PLACEHOLDER NOTE: show RDMA throughput curve]

Figure placeholder: TCP IO graph (1 GB)
[FIGURE: tcp_io_graph.png]
[PLACEHOLDER NOTE: show TCP throughput curve]

Figure placeholder: Conversations throughput table
[FIGURE: conversations_throughput.png]
[PLACEHOLDER NOTE: highlight bytes/s for RDMA vs TCP]

## 8) Where to go next
- Deep dive into `src/` for the full modular flow.
- Use `examples/ai-ml/README.md` to evolve the protocol into richer workloads.
- Keep the Wireshark captures in the post; they make the protocol tangible.

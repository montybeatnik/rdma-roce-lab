# Tutorial narrative: RDMA walkthrough with real-world anchors

This narrative is aimed at network engineers who want to understand the
application-level protocol that RDMA enables, while staying accessible to ML
engineers and general readers.

## 1) What RDMA gives you (and what it requires)
RDMA allows one host to read/write memory on another host without involving the
remote CPU in the data path. The "price of admission" is that both sides agree
on memory registration and exchange a small capability blob.

That blob has two critical fields:
- `addr`: remote virtual address for the buffer.
- `rkey`: a capability token that authorizes access to that buffer.

In this repo, the server sends `{addr, rkey}` in `private_data` during the RDMA
CM connection handshake. The client uses those values to post one-sided ops.

## 2) Why the setup looks the way it does
The lab uses two VMs and SoftRoCE so the RDMA protocol can run on a normal NIC:

- **Two VMs**: keeps server and client cleanly separated, like production.
- **SoftRoCE (`rdma_rxe`)**: provides RDMA semantics on a standard Ethernet NIC.
- **`librdmacm`**: handles the control plane (address resolution, connect).
- **`libibverbs`**: handles the data plane (WRITE/READ work requests).

You can see this setup in `scripts/guide/01_multipass_setup.sh`.

## 3) The minimal flow (for blog snippets)
The minimal example lives in `examples/c/minimal/` and focuses on the essential
control and data plane steps.

### Server: register a buffer and share its capability
```c
alloc_and_reg(&c, &c.buf_remote, &c.mr_remote, BUF_SZ,
              IBV_ACCESS_LOCAL_WRITE | 
              IBV_ACCESS_REMOTE_READ |
              IBV_ACCESS_REMOTE_WRITE);

struct remote_buf_info info =
    pack_remote_buf_info((uintptr_t)c.buf_remote, c.mr_remote->rkey);
    cm_server_accept_with_priv(&c,  &info, sizeof(info));
```

Why it matters: this is the memory-safety contract. The server opts in by
registering a buffer and sharing only the exact capability needed.

### Client: use the capability to WRITE and READ
```c
unpack_remote_buf_info(&info, &c.remote_addr, &c.remote_rkey);

post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
           strlen((char *)c.buf_tx) + 1, 1, 1);
post_read(c.qp, c.mr_rx, c.buf_rx, c.remote_addr, c.remote_rkey, BUF_SZ, 2, 1);
```

Why it matters: the remote CPU is not on the data path. The network fabric
enforces the access via the `rkey`.

## 4) Real-world anchors (why RDMA looks this way)

### Parameter servers (training)
- **RDMA WRITE**: workers push gradient updates into a shared buffer.
- **RDMA READ**: workers pull updated weights back.
- **Why this design**: RDMA minimizes CPU overhead and tail latency during
  synchronization points in training.

### Distributed training rendezvous
- **WRITE_WITH_IMM**: send a small "ready" signal alongside data.
- **Why this design**: immediate data provides a low-overhead notification path
  without a full send/recv protocol.

Short callout (client side):
```c
uint32_t imm = htonl(1); // "ready" flag for rendezvous
CHECK(post_write_with_imm(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                          payload_len, 2, imm),
      "post_write_with_imm");
```
Full context: `src/client_imm.c`

### Inference batch pipelines
- **WRITE**: producer writes a batch into a ring buffer.
- **WRITE_WITH_IMM**: producer notifies the consumer of a new batch id.
- **Why this design**: avoids copying and keeps latency predictable under load.

Short callout (client side):
```c
CHECK(post_write(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                 batch_bytes, 1, 1),
      "post_write");
uint32_t imm = htonl(batch_id); // notify consumer which batch landed
CHECK(post_write_with_imm(c.qp, c.mr_tx, c.buf_tx, c.remote_addr, c.remote_rkey,
                          0, 2, imm),
      "post_write_with_imm");
```
Full context: `src/client_imm.c`

## 5) Suggested narrative order for the blog
1) Explain the "capability blob" (`addr` + `rkey`) and why it exists.
2) Walk through `examples/c/minimal` in 15–20 lines of code.
3) Show how the same flow maps to parameter servers and inference pipelines.
4) Link to the fuller examples in `src/` for readers who want depth.

## Navigation
- Up: [Docs index](README.md)

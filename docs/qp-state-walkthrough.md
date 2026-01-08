# QP state walkthrough (minimal lab)

This is a concrete, capture‑friendly walkthrough of the RC QP lifecycle using the minimal lab.

## 1) Start the server

```bash
./rdma_min_server 7471
```

The server binds and listens. At this point its QP is not usable for data.

Capture slot: `[Screenshot: server listen log]`

## 2) Start the client

```bash
./rdma_min_client <SERVER_IP> 7471
```

The client resolves address and route, then creates PD/CQ/QP.

Capture slot: `[Screenshot: client resolve log]`

## 3) QP state transitions (RC)

Typical RC state progression:

- **RESET**: no trust, no remote info
- **INIT**: local intent (PD, port, access flags)
- **RTR**: remote is known (path + permissions locked)
- **RTS**: ready to send/receive; RNIC can act autonomously

These transitions are driven by CM events during connect/accept.

Capture slot: `[Screenshot: QP state log / dump]`

## 4) CM traffic vs data traffic

RC setup is coordinated via **RDMA CM** (UDP/4791 for RoCEv2). Once established, data traffic uses the RDMA data plane.

Suggested filters:
```text
udp.port == 4791
eth.type == 0x8915
```

Capture slot: `[Screenshot: CM handshake packets]`
Capture slot: `[Screenshot: data plane packets]`

## 5) Verify flow

On the client, the minimal example posts WRITE then READ and polls CQ for completions.

Capture slot: `[Screenshot: CQE log output]`

## Visual companion

Open `sims/qp_state_machine.html` for a visual of state progression and CM/data separation.

## Navigation

- Previous: [On the wire](on-the-wire.md)
- Next: [Guided scripts](guide-scripts.md)

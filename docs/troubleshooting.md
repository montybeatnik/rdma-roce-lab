# Troubleshooting matrix

Common issues in the lab, with likely causes and concrete fixes.

| Symptom | Likely cause | Fix / check |
| --- | --- | --- |
| `rdma_resolve_addr: No such device` | RDMA netns support missing, or rxe device not created inside the container netns | Verify `/proc/sys/net/rdma/enable_rdma_netns` exists. Run `rdma link` inside the container; recreate with `rdma link add rxe0 type rxe netdev eth1`. |
| `RDMA_CM_EVENT_UNREACHABLE` or timeout | CM packets (UDP 4791) blocked or routed wrong | From client: `ip route get <server-ip>` and `ping <server-ip>`. Capture `udp port 4791` on the correct interface. |
| No CM packets in tcpdump | Capturing the wrong interface or wrong namespace | Run `ip -o route get <server-ip>` and capture on that device. For containerlab, capture inside the container netns. |
| `rdma link add rxe-*` -> `Wrong device name` | Name collision or device already created in another namespace | List existing devices with `rdma link`. Delete in the correct namespace: `rdma link delete rxe-*`. Use a unique name per netns. |
| `rdma link add rxe-*` -> `File exists` | rxe device already exists | `rdma link` to confirm, then delete and recreate if it is bound to the wrong netdev. |
| QP stuck in INIT/RTR, client hangs at `wait ESTABLISHED` | Server not listening, CM connect not completing, or CM not routed | Confirm server is running and bound to the right IP. Verify `ss -lntp | rg 7471` on server VM. Check CM packets on UDP/4791. |
| Client WRITE completes but server sees no CQE | Missing RECV for SEND/WRITE_WITH_IMM or wrong CQ signaling | Ensure server posted a RECV where required, and that the WR uses `IBV_SEND_SIGNALED` periodically. |
| CQ polling spins forever | No completions posted or signaled | Ensure WRs are signaled, and check for `wc.status != IBV_WC_SUCCESS`. |
| Netem shows zero drops | Netem applied to the wrong interface | Run `ip route get <peer-ip>` and apply `tc qdisc` to that device. Use `tc -s qdisc show dev <dev>` to confirm drops. |
| `ibv_devices` in container shows host rxe devices | RDMA netns disabled | Use a kernel with RDMA netns support or run VM-only labs. |
| `cannot execute binary file` when running scripts | Built on host, executed inside VM | Build inside the VM using `scripts/guide/02_build_c.sh`. |

## Navigation
- Previous: [Prereqs](prereqs.md)
- Next: [On the wire](on-the-wire.md)

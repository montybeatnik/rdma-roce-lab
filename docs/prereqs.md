# Prereqs and known-good platform

This page summarizes what the lab assumes about your kernel, RDMA stack, and
runtime environment. It also lists the failure modes you will see when those
assumptions do not hold.

## Quick check (5 minutes)

Run inside a VM or host where you plan to run the lab:

```bash
uname -r
rdma -V
ibv_devices
lsmod | rg 'rdma_rxe|ib_core|ib_uverbs' || true
```

If `ibv_devices` shows at least `rxe0`, you are ready for the VM-only labs.

## Kernel support for RDMA in network namespaces (***for containerlab topo only***)

The container fabric needs RDMA devices to be isolated per netns. That requires
kernel support for RDMA netns.

Check for the sysctl:

```bash
ls /proc/sys/net/rdma/enable_rdma_netns
```

Check kernel config:

```bash
grep CONFIG_RDMA_NETNS /boot/config-$(uname -r) || true
zgrep CONFIG_RDMA_NETNS /proc/config.gz 2>/dev/null || true
```

Expected:
- `CONFIG_RDMA_NETNS=y`
- `/proc/sys/net/rdma/enable_rdma_netns` exists

If these are missing, RDMA devices are global across netns. The container
fabric will not behave correctly.

## Known-good platform (requirements, not a single image)

This lab works reliably when all of the following are true:
- Kernel has RDMA netns enabled (`CONFIG_RDMA_NETNS=y`).
- `rdma_rxe` module is available and loadable.
- `rdma-core` userspace tools are installed (`rdma`, `ibv_*`).
- The VM has a reachable data interface between client and server VMs.

If you cannot meet the RDMA netns requirement, use the VM-only labs (no
container fabric) and skip containerlab-based flows.

## Common failure modes tied to prereqs

Symptoms you will see if prerequisites are missing:
- `rdma_resolve_addr: No such device` inside a container: RDMA netns support
  missing or rxe not created in that namespace.
- `rdma link add rxe-*` returns `Wrong device name` or `File exists`: name
  collision or device created in another namespace.
- `ibv_devices` shows only `rxe0` inside containers even after creating
  `rxe-gpu1`/`rxe-gpu2`: RDMA netns support is missing.

## Navigation
- Previous: [Lab setup](lab-setup.md)
- Next: [Troubleshooting](troubleshooting.md)

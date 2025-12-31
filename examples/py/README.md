# Python (pyverbs) examples

> **Experimental / WIP — may fail with tracebacks.** Use the C labs first. See [docs/python-status.md](../../docs/python-status.md).

These are small, readable scripts that use `pyverbs` (from `rdma-core`) to show
the same building blocks as the C samples. They are intentionally tiny so you
can see the shape of the API before we connect real QPs.

## If it fails, do this instead (stable path)

- `ibv_devices` and `ibv_devinfo` for device visibility and port state.
- `rdma link` to check `rxe0` presence and configuration.
- `examples/c/minimal/README.md` for the supported C minimal flow.

## Prereqs (Ubuntu)
```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pyverbs
```

## Create a venv
From the repo root:
```bash
bash scripts/guide/00_setup_venv.sh
source .venv/bin/activate
```

## What you can run now
These are experimental and may fail depending on your distro and `rdma-core`.

- `examples/py/00_list_devices.py` shows what RDMA devices are visible (may fail).
- `examples/py/01_query_ports.py` prints a device's port attributes (may fail).
- `examples/py/02_rc_write_skeleton.py` sets up PD/CQ/MR/QP and shows a
  two-host RDMA WRITE skeleton (may fail).
- `examples/py/10_minimal_server.py` and `examples/py/11_minimal_client.py`
  mirror the minimal C flow using RDMA CM + private_data (may fail).

## Example
```bash
# All commands below may fail depending on pyverbs packaging and rdma-core version.
python3 examples/py/00_list_devices.py
python3 examples/py/01_query_ports.py rxe0 1
python3 examples/py/02_rc_write_skeleton.py rxe0 1
python3 examples/py/10_minimal_server.py 7471
python3 examples/py/11_minimal_client.py <SERVER_IP> 7471
```

## Make targets (optional)
```bash
# All commands below may fail depending on pyverbs packaging and rdma-core version.
make py-list-devices
make py-query-ports PY_DEV=rxe0 PY_PORT=1
make py-minimal-server PY_CM_PORT=7471
make py-minimal-client PY_SERVER_IP=<SERVER_IP> PY_CM_PORT=7471
```

## RDMA CM note
The minimal server/client uses RDMA CM for connection setup and uses
`private_data` to pass the remote buffer info (addr, rkey, len), just like the
C minimal example.

## Why start here?
Before we post work requests, we need a reliable mental model of the device and
port we are talking to. Think of this as the “verify the instruments” step.

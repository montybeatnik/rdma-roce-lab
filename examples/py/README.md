# Python (pyverbs) examples

These are small, readable scripts that use `pyverbs` (from `rdma-core`) to show
the same building blocks as the C samples. They are intentionally tiny so you
can see the shape of the API before we connect real QPs.

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
- `examples/py/00_list_devices.py` shows what RDMA devices are visible.
- `examples/py/01_query_ports.py` prints a device's port attributes.
- `examples/py/02_rc_write_skeleton.py` sets up PD/CQ/MR/QP and shows a
  two-host RDMA WRITE skeleton.

## Example
```bash
python3 examples/py/00_list_devices.py
python3 examples/py/01_query_ports.py rxe0 1
python3 examples/py/02_rc_write_skeleton.py rxe0 1
```

## Why start here?
Before we post work requests, we need a reliable mental model of the device and
port we are talking to. Think of this as the “verify the instruments” step.

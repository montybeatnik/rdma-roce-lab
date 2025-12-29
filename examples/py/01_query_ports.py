#!/usr/bin/env python3
import sys

try:
    import pyverbs.device as pv_dev
    from pyverbs.device import Context
except ImportError as exc:
    print("pyverbs is not installed. Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc


def _list_devices():
    if hasattr(pv_dev, "DeviceList"):
        return pv_dev.DeviceList()
    if hasattr(pv_dev, "get_device_list"):
        return pv_dev.get_device_list()
    raise RuntimeError("pyverbs.device has no DeviceList or get_device_list")


def _device_name(dev):
    if hasattr(dev, "name"):
        val = dev.name
        return val() if callable(val) else val
    if hasattr(dev, "device_name"):
        val = dev.device_name
        return val() if callable(val) else val
    return str(dev)


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: 01_query_ports.py <device_name> <port>")
        return 1
    dev_name = sys.argv[1]
    port = int(sys.argv[2])

    dev_list = _list_devices()
    dev = next((d for d in dev_list if _device_name(d) == dev_name), None)
    if dev is None:
        print(f"Device '{dev_name}' not found.")
        return 1

    ctx = Context(dev)
    dev_attr = ctx.query_device()
    port_attr = ctx.query_port(port)

    print(f"Device: {dev_name}")
    print(f"  max_qp: {dev_attr.max_qp}")
    print(f"  max_cq: {dev_attr.max_cq}")
    print(f"Port {port}:")
    print(f"  state: {port_attr.state}")
    print(f"  max_mtu: {port_attr.max_mtu}")
    print(f"  active_mtu: {port_attr.active_mtu}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

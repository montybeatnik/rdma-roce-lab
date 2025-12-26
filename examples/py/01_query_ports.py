#!/usr/bin/env python3
import sys

try:
    from pyverbs.device import Context, DeviceList
except ImportError as exc:
    print("pyverbs is not installed. Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: 01_query_ports.py <device_name> <port>")
        return 1
    dev_name = sys.argv[1]
    port = int(sys.argv[2])

    dev_list = DeviceList()
    dev = next((d for d in dev_list if d.name == dev_name), None)
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

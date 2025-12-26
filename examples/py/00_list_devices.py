#!/usr/bin/env python3
import sys

try:
    from pyverbs.device import DeviceList
except ImportError as exc:
    print("pyverbs is not installed. Try: sudo apt install -y python3-pyverbs")
    raise SystemExit(1) from exc


def main() -> int:
    dev_list = DeviceList()
    if len(dev_list) == 0:
        print("No RDMA devices found.")
        return 1
    print("RDMA devices:")
    for dev in dev_list:
        print(f"- {dev.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

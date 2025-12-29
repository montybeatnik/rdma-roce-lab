#!/usr/bin/env python3
import sys

try:
    import pyverbs.device as pv_dev
except Exception as exc:
    print("pyverbs import failed.")
    print(f"Error: {exc}")
    print("Try: sudo apt install -y python3-pyverbs")
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
    dev_list = _list_devices()
    if len(dev_list) == 0:
        print("No RDMA devices found.")
        return 1
    print("RDMA devices:")
    for dev in dev_list:
        print(f"- {_device_name(dev)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

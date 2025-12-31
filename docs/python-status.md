# Python (pyverbs) status: Experimental

Python examples in this repo are **experimental**. They are useful for learning the API shape, but they are not the primary supported path. The C labs are the stable, default path.

Why pyverbs can fail (common causes):
- `rdma-core` and `pyverbs` ABI mismatch across distros or versions.
- Missing provider libraries (device support not present).
- Packaging differences between `apt` and `pip`.
- Kernel/userspace mismatches in lab VMs.

What is supported today:
- **C labs are the supported path.**
- **Python is optional and may fail with tracebacks** depending on your setup.

How to report a Python issue:

```
OS version:
Python version:
pyverbs installed via: (apt/pip)
rdma-core version:
Command run:
Full traceback:
```

Back to the main README: [README.md](../README.md)

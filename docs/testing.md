# Testing

The tests are intentionally small and split into unit and integration groups.

## Unit tests (no RDMA device required)
```bash
make tests
```
This runs:
- tests/test_endian: endian helpers and private_data packing.
- tests/test_mem: alignment and allocation sanity checks.

## Integration tests (requires RDMA device)
```bash
make tests SERVER_IP=<server-ip>
```
This runs `tests/test_run_integration.sh` which:
- Starts Example 1 server/client and checks for WRITE/READ completions.
- Starts Example 2 server/client and checks for WRITE_WITH_IMM + RECV.

If no RDMA device is present, the integration test will skip.

## Python compatibility tests
These validate the pyverbs compatibility helpers without needing RDMA hardware.

Install pytest:
```bash
python -m pip install pytest
```

Run:
```bash
make py-tests
```

## Navigation
- Previous: [AI/ML use cases](ai-ml-use-cases.md)
- Next: [Blog post ideas](blog-post-ideas.md)

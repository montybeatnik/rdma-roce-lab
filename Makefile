
CC=gcc
CFLAGS=-O2 -std=c11 -Wall -D_GNU_SOURCE -DRDMA_VERBOSE
LDFLAGS=-lrdmacm -libverbs
PYTHON?=python3
PYTEST?=$(PYTHON) -m pytest
PY_DEV?=rxe0
PY_PORT?=1
PY_CM_PORT?=7471
PY_SERVER_IP?=

SRC_DIR=src
BIN_DIR=.

SRCS=$(SRC_DIR)/common.c $(SRC_DIR)/rdma_cm_helpers.c $(SRC_DIR)/rdma_builders.c $(SRC_DIR)/rdma_mem.c $(SRC_DIR)/rdma_ops.c
HDRS=$(SRC_DIR)/common.h $(SRC_DIR)/rdma_ctx.h $(SRC_DIR)/rdma_cm_helpers.h $(SRC_DIR)/rdma_builders.h $(SRC_DIR)/rdma_mem.h $(SRC_DIR)/rdma_ops.h

all: rdma_server rdma_client rdma_server_imm rdma_client_imm minimal rdma_bulk_server rdma_bulk_client tcp_server tcp_client mr_cache

rdma_server: $(SRCS) $(SRC_DIR)/server_main.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/server_main.c -o $@ $(LDFLAGS)

rdma_client: $(SRCS) $(SRC_DIR)/client_main.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/client_main.c -o $@ $(LDFLAGS)

rdma_server_imm: $(SRCS) $(SRC_DIR)/server_imm.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/server_imm.c -o $@ $(LDFLAGS)

rdma_client_imm: $(SRCS) $(SRC_DIR)/client_imm.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/client_imm.c -o $@ $(LDFLAGS)

rdma_bulk_server: $(SRCS) examples/c/rdma-bulk/rdma_bulk_server.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/rdma-bulk/rdma_bulk_server.c -o $@ $(LDFLAGS)

rdma_bulk_client: $(SRCS) examples/c/rdma-bulk/rdma_bulk_client.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/rdma-bulk/rdma_bulk_client.c -o $@ $(LDFLAGS)

tcp_server: examples/c/tcp/tcp_server.c examples/c/tcp/tcp_common.h
	$(CC) $(CFLAGS) examples/c/tcp/tcp_server.c -o $@

tcp_client: examples/c/tcp/tcp_client.c examples/c/tcp/tcp_common.h
	$(CC) $(CFLAGS) examples/c/tcp/tcp_client.c -o $@

perf-compare:
	@echo "[INFO] RDMA bulk (1G) and TCP (1G) comparison"
	@echo "Server VM (rdma-server):"
	@echo "  ./rdma_bulk_server 7471 1G | tee /tmp/rdma_1g_server.log"
	@echo "  ./tcp_server 9000 1G       | tee /tmp/tcp_1g_server.log"
	@echo "Client VM (rdma-client):"
	@echo "  ./rdma_bulk_client <SERVER_IP> 7471 1G 4M | tee /tmp/rdma_1g_client.log"
	@echo "  ./tcp_client <SERVER_IP> 9000 1G          | tee /tmp/tcp_1g_client.log"
minimal_server: $(SRCS) examples/c/minimal/server_min.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/minimal/server_min.c -o rdma_min_server $(LDFLAGS)

minimal_client: $(SRCS) examples/c/minimal/client_min.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/minimal/client_min.c -o rdma_min_client $(LDFLAGS)

minimal: minimal_server minimal_client

mr_cache_server: $(SRCS) examples/c/mr-cache/server_mr_cache.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/mr-cache/server_mr_cache.c -o $@ $(LDFLAGS)

mr_cache_client: $(SRCS) examples/c/mr-cache/client_mr_cache.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) examples/c/mr-cache/client_mr_cache.c -o $@ $(LDFLAGS)

mr_cache: mr_cache_server mr_cache_client

clean:
	rm -f rdma_server rdma_client rdma_server_imm rdma_client_imm rdma_min_server rdma_min_client \
		rdma_bulk_server rdma_bulk_client tcp_server tcp_client mr_cache_server mr_cache_client

# ---- Tests ----
TESTS_DIR=tests
UNIT_TESTS=$(TESTS_DIR)/test_endian $(TESTS_DIR)/test_mem

$(TESTS_DIR)/test_endian: $(TESTS_DIR)/test_endian.c $(SRC_DIR)/common.h
	$(CC) $(CFLAGS) -I$(SRC_DIR) $< -o $@

$(TESTS_DIR)/test_mem: $(TESTS_DIR)/test_mem.c
	$(CC) $(CFLAGS) $< -o $@

tests: $(UNIT_TESTS)
	@echo "[RUN] unit: test_endian"; $(TESTS_DIR)/test_endian
	@echo "[RUN] unit: test_mem";    $(TESTS_DIR)/test_mem
	@echo "[RUN] integration (log-based)"; $(TESTS_DIR)/test_run_integration.sh $(SERVER_IP)

test: tests

py-tests:
	@$(PYTEST) -q

# ---- Capture ----
CAPTURE_HOST ?= rdma-server
CAPTURE_CLIENT_HOST ?= rdma-client
CAPTURE_USER ?= ubuntu
CAPTURE_SECONDS ?= 10
CAPTURE_REMOTE ?= /home/ubuntu/rdma-roce-lab/roce_capture.pcap
CAPTURE_LOCAL ?=
CAPTURE_REPO ?= /home/ubuntu/rdma-roce-lab
CAPTURE_PORT ?= 7471
CAPTURE_BUILD ?= 1
CAPTURE_FETCH_LOGS ?= 1
CAPTURE_LIVE ?= 0
CAPTURE_WIRESHARK_BIN ?=
CAPTURE_NETNS ?=
CAPTURE_IFACE ?=
CAPTURE_FILTER ?= udp port 4791 or ether proto 0x8915 or arp or icmp
CAPTURE_SERVER_WAIT_SECONDS ?= 3
CAPTURE_RDMA_DEV ?= rxe0
CAPTURE_SERVER_IP ?=
CAPTURE_CLIENT_DELAY_SECONDS ?= 0
CAPTURE_CLIENT_IP ?=
CAPTURE_BIND_IP ?= off
CAPTURE_CLIENT_SRC_IP ?= off
CAPTURE_SERVER_RUN_SECONDS ?=
CAPTURE_CLIENT_RETRIES ?= 2
CAPTURE_CLIENT_RETRY_DELAY ?= 2
CAPTURE_INITIATOR_DEPTH ?=
CAPTURE_RESPONDER_RESOURCES ?=
CAPTURE_USE_SCRIPTS ?= 1
CAPTURE_SHOW_HOST_INFO ?= 1
CAPTURE_RUN_APPS ?= 1

lab-capture:
	@CAPTURE_HOST="$(CAPTURE_HOST)" \
	 CAPTURE_CLIENT_HOST="$(CAPTURE_CLIENT_HOST)" \
	 CAPTURE_USER="$(CAPTURE_USER)" \
	 CAPTURE_SECONDS="$(CAPTURE_SECONDS)" \
	 CAPTURE_REMOTE="$(CAPTURE_REMOTE)" \
	 CAPTURE_LOCAL="$(CAPTURE_LOCAL)" \
	 CAPTURE_REPO="$(CAPTURE_REPO)" \
	 CAPTURE_PORT="$(CAPTURE_PORT)" \
	 CAPTURE_BUILD="$(CAPTURE_BUILD)" \
	 CAPTURE_FETCH_LOGS="$(CAPTURE_FETCH_LOGS)" \
	 CAPTURE_LIVE="$(CAPTURE_LIVE)" \
	 CAPTURE_WIRESHARK_BIN="$(CAPTURE_WIRESHARK_BIN)" \
	 CAPTURE_NETNS="$(CAPTURE_NETNS)" \
	 CAPTURE_IFACE="$(CAPTURE_IFACE)" \
	 CAPTURE_FILTER="$(CAPTURE_FILTER)" \
	 CAPTURE_SERVER_WAIT_SECONDS="$(CAPTURE_SERVER_WAIT_SECONDS)" \
	 CAPTURE_RDMA_DEV="$(CAPTURE_RDMA_DEV)" \
	 CAPTURE_SERVER_IP="$(CAPTURE_SERVER_IP)" \
	 CAPTURE_CLIENT_DELAY_SECONDS="$(CAPTURE_CLIENT_DELAY_SECONDS)" \
	 CAPTURE_CLIENT_IP="$(CAPTURE_CLIENT_IP)" \
	 CAPTURE_BIND_IP="$(CAPTURE_BIND_IP)" \
	 CAPTURE_CLIENT_SRC_IP="$(CAPTURE_CLIENT_SRC_IP)" \
	 CAPTURE_SERVER_RUN_SECONDS="$(CAPTURE_SERVER_RUN_SECONDS)" \
	 CAPTURE_CLIENT_RETRIES="$(CAPTURE_CLIENT_RETRIES)" \
	 CAPTURE_CLIENT_RETRY_DELAY="$(CAPTURE_CLIENT_RETRY_DELAY)" \
	 CAPTURE_INITIATOR_DEPTH="$(CAPTURE_INITIATOR_DEPTH)" \
	 CAPTURE_RESPONDER_RESOURCES="$(CAPTURE_RESPONDER_RESOURCES)" \
	 CAPTURE_USE_SCRIPTS="$(CAPTURE_USE_SCRIPTS)" \
	 CAPTURE_SHOW_HOST_INFO="$(CAPTURE_SHOW_HOST_INFO)" \
	 CAPTURE_RUN_APPS="$(CAPTURE_RUN_APPS)" \
	 bash scripts/lab_capture.sh

lab-capture-manual:
	@CAPTURE_RUN_APPS=0 $(MAKE) lab-capture

lab-capture-live-manual:
	@CAPTURE_RUN_APPS=0 CAPTURE_LIVE=1 $(MAKE) lab-capture

lab-capture-live:
	@CAPTURE_LIVE=1 $(MAKE) lab-capture

# ---- Multipass lab ----
lab-deploy:
	bash scripts/guide/01_multipass_setup.sh

# To test 1G transfer
# make lab-clean
# CPUS=4 MEM=8G DISK=40G bash scripts/guide/01_multipass_setup.sh


lab-clean:
	multipass delete rdma-server rdma-client && multipass purge

py-list-devices:
	$(PYTHON) examples/py/00_list_devices.py

py-query-ports:
	$(PYTHON) examples/py/01_query_ports.py $(PY_DEV) $(PY_PORT)

py-minimal-server:
	$(PYTHON) examples/py/10_minimal_server.py $(PY_CM_PORT)

py-minimal-client:
	@if [ -z "$(PY_SERVER_IP)" ]; then \
		echo "PY_SERVER_IP is required (e.g., make py-minimal-client PY_SERVER_IP=1.2.3.4)"; \
		exit 1; \
	fi
	$(PYTHON) examples/py/11_minimal_client.py $(PY_SERVER_IP) $(PY_CM_PORT)

.PHONY: tests test minimal minimal_server minimal_client rdma_bulk_server rdma_bulk_client tcp_server tcp_client \
	mr_cache mr_cache_server mr_cache_client \
	perf-compare lab-capture lab-capture-live lab-capture-manual lab-capture-live-manual lab-deploy lab-clean \
	py-list-devices py-query-ports py-minimal-server py-minimal-client py-tests


CC=gcc
CFLAGS=-O2 -std=c11 -Wall
LDFLAGS=-lrdmacm -libverbs

SRC_DIR=src
BIN_DIR=.

SRCS=$(SRC_DIR)/rdma_cm_helpers.c $(SRC_DIR)/rdma_builders.c $(SRC_DIR)/rdma_mem.c $(SRC_DIR)/rdma_ops.c
HDRS=$(SRC_DIR)/common.h $(SRC_DIR)/rdma_ctx.h $(SRC_DIR)/rdma_cm_helpers.h $(SRC_DIR)/rdma_builders.h $(SRC_DIR)/rdma_mem.h $(SRC_DIR)/rdma_ops.h

all: rdma_server rdma_client rdma_server_imm rdma_client_imm

rdma_server: $(SRCS) $(SRC_DIR)/server_main.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/server_main.c -o $@ $(LDFLAGS)

rdma_client: $(SRCS) $(SRC_DIR)/client_main.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/client_main.c -o $@ $(LDFLAGS)

rdma_server_imm: $(SRCS) $(SRC_DIR)/server_imm.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/server_imm.c -o $@ $(LDFLAGS)

rdma_client_imm: $(SRCS) $(SRC_DIR)/client_imm.c $(HDRS)
	$(CC) $(CFLAGS) -I$(SRC_DIR) $(SRCS) $(SRC_DIR)/client_imm.c -o $@ $(LDFLAGS)

clean:
	rm -f rdma_server rdma_client rdma_server_imm rdma_client_imm

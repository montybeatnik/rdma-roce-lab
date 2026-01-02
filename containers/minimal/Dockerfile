FROM ubuntu:22.04 AS build

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential librdmacm-dev libibverbs-dev pkg-config && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . /src
RUN make minimal

FROM ubuntu:22.04
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      iproute2 iputils-ping rdma-core ibverbs-utils ibverbs-providers \
      librdmacm1 libibverbs1 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /src/rdma_min_server /usr/local/bin/rdma_min_server
COPY --from=build /src/rdma_min_client /usr/local/bin/rdma_min_client
COPY containers/minimal/run.sh /usr/local/bin/rdma_min_run
RUN chmod +x /usr/local/bin/rdma_min_run

ENTRYPOINT ["/usr/local/bin/rdma_min_run"]

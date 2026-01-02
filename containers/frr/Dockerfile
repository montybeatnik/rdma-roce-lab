FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends frr frr-pythontools iproute2 && \
    rm -rf /var/lib/apt/lists/*

COPY containers/frr/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

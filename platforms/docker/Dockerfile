FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
ARG HUSARNET_ARCH

RUN apt-get update -y

COPY ./build/release/husarnet-linux-${HUSARNET_ARCH}.deb /tmp/husarnet.deb
RUN apt-get install -y --no-install-recommends --no-install-suggests \
    /tmp/husarnet.deb && rm -f /tmp/husarnet.deb

COPY ./platforms/docker/husarnet-docker.sh /usr/bin/husarnet-docker
COPY ./platforms/docker/husarnet-docker-healthcheck.sh /usr/bin/husarnet-docker-healthcheck

SHELL ["/usr/bin/bash", "-c"]
HEALTHCHECK --interval=10s --timeout=65s --start-period=5s --retries=6 CMD husarnet-docker-healthcheck || exit 1
CMD husarnet-docker

FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y

COPY ./build/release/husarnet-linux.deb /tmp
RUN apt install -y --no-install-recommends --no-install-suggests \
    /tmp/husarnet-linux.deb && rm -f /tmp/husarnet-linux.deb

COPY ./platforms/docker/husarnet-docker.sh /usr/bin/husarnet-docker
COPY ./platforms/docker/husarnet-docker-healthcheck.sh /usr/bin/husarnet-docker-healthcheck

SHELL ["/usr/bin/bash", "-c"]
HEALTHCHECK --interval=10s --timeout=65s --start-period=5s --retries=6 CMD husarnet-docker-healthcheck || exit 1
CMD husarnet-docker

FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && apt-get install -y iptables procps iproute2 libubsan1
RUN update-alternatives --set ip6tables /usr/sbin/ip6tables-nft

COPY ./platforms/docker/husarnet-docker.sh /usr/bin/husarnet-docker
COPY ./platforms/docker/husarnet-docker-healthcheck.sh /usr/bin/husarnet-docker-healthcheck

COPY ./build/release/husarnet-daemon /usr/bin/husarnet-daemon
COPY ./build/release/husarnet /usr/bin/husarnet

HEALTHCHECK --interval=10s --timeout=65s --start-period=5s --retries=6 CMD [ "husarnet-docker-healthcheck" ] || exit 1
SHELL ["bash"]
CMD husarnet-docker

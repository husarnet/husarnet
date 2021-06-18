FROM debian:stable-slim
ARG build_arch

RUN apt-get update -y && apt-get install -y iptables procps iproute2
RUN update-alternatives --set ip6tables /usr/sbin/ip6tables-nft

COPY husarnet-unix-${build_arch} /usr/bin/husarnet
COPY husarnet-docker /usr/bin/husarnet-docker
CMD husarnet-docker

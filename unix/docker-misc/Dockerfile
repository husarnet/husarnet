FROM ubuntu:hirsute AS app_builder
ARG TARGETPLATFORM
RUN echo $TARGETPLATFORM

RUN apt-get update -y && apt-get install -y git
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends python3 linux-headers-generic ninja-build cmake build-essential g++-mingw-w64
RUN ln -sf /usr/include/asm-generic /usr/include/asm

WORKDIR /husarnet
# note: will assume context as . (repo root)
COPY . .
RUN git submodule update --init --recursive
RUN ./util/build-cmake.sh $TARGETPLATFORM

# stage 2
FROM ubuntu:hirsute
RUN apt-get update -y && apt-get install -y iptables procps iproute2
RUN update-alternatives --set ip6tables /usr/sbin/ip6tables-nft

COPY --from=app_builder /husarnet/build/bin/husarnet-daemon /usr/bin/husarnet-daemon
COPY ./unix/docker-misc/husarnet-docker.sh /usr/bin/husarnet-docker

CMD husarnet-docker

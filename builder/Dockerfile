FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
    bind9-host \
    binutils \
    clang-format \
    cmake \
    cppcheck \
    file \
    git \
    golang \
    mc \
    ninja-build \
    patch \
    python3 \
    rpm \
    ruby \
    rubygems \
    sudo \
    tar \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN gem install --no-document fpm

WORKDIR /zig
RUN wget -O zig.tar.xz "https://ziglang.org/download/0.9.1/zig-linux-x86_64-0.9.1.tar.xz" \
    && tar -xf zig.tar.xz \
    && rm -f zig.tar.xz \
    && ln -s /zig/zig-linux-x86_64-0.9.1/zig /usr/bin/zig

WORKDIR /jsonnet
RUN wget -O jsonnet.tar.gz "https://github.com/google/go-jsonnet/releases/download/v0.19.1/go-jsonnet_0.19.1_Linux_x86_64.tar.gz" \
    && tar -xf jsonnet.tar.gz \
    && rm -f jsonnet.tar.gz \
    && ln -s /jsonnet/jsonnet /usr/bin/ \
    && ln -s /jsonnet/jsonnetfmt /usr/bin/

RUN groupadd builder \
    && useradd -m -u 1000 -g 1000 builder \
    && echo "builder ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

ENV CGO_ENABLED="0"
ENV GOCACHE="/app/build/gocache"
ENV GOFLAGS="-buildvcs=false"

WORKDIR /app
ENTRYPOINT ["/app/builder/entrypoint.sh"]

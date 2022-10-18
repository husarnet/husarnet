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

RUN groupadd builder \
    && useradd -m -u 1000 -g 1000 builder \
    && echo "builder ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

ENV CGO_ENABLED="0"
ENV GOCACHE="/app/build/gocache"
ENV GOFLAGS="-buildvcs=false"

WORKDIR /app
ENTRYPOINT ["/app/builder/entrypoint.sh"]

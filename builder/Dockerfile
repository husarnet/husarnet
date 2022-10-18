FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ENV DEBCONF_NONINTERACTIVE_SEEN true

RUN apt-get update

RUN apt-get install -y \
    bind9-host \
    build-essential \
    bzip2 \
    ca-certificates \
    clang-format \
    cmake \
    cppcheck \
    diffutils \
    file \
    g++ \
    gcc \
    git \
    golang \
    linux-headers-generic \
    make \
    mc \
    ninja-build \
    patch \
    python3 \
    rpm \
    rsync \
    ruby \
    ruby-dev \
    rubygems \
    sudo \
    texinfo \
    unzip \
    wget \
    xz-utils

RUN sudo gem install --no-document fpm

WORKDIR /zig
RUN wget -O zig.tar.xz "https://ziglang.org/download/0.9.1/zig-linux-x86_64-0.9.1.tar.xz" && tar -xf zig.tar.xz
ENV PATH="${PATH}:/zig/zig-linux-x86_64-0.9.1"

RUN groupadd builder && useradd -m -u 1000 -g 1000 builder
USER 1000:1000

USER 0
RUN echo "builder   ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers
USER 1000
WORKDIR /app

ENV CGO_ENABLED=0
ENV GOCACHE=/app/build/gocache
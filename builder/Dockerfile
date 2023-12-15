FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
    bind9-host \
    binutils \
    bison \
    ca-certificates \
    ccache \
    clang \
    clang-format \
    cmake \
    cppcheck \
    cpplint \
    dfu-util \
    file \
    flex \
    gdb \
    git \
    golang \
    gperf \
    libarchive-tools \
    libffi-dev \
    libssl-dev \
    libusb-1.0-0 \
    mc \
    ninja-build \
    openssl \
    patch \
    python3 \
    python3-pip \
    python3-venv \
    rpm \
    ruby \
    rubygems \
    sudo \
    tar \
    tree \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN gem install --no-document fpm

WORKDIR /zig
# https://github.com/ziglang/zig/pull/16109
# Keeping zig at 0.9.1 temporarily due to this
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
ENV GOPATH="/app/build/gopath"
ENV GOFLAGS="-buildvcs=false"

# RUN update-alternatives --install /usr/bin/python python /usr/bin/python3 10
# RUN pip3 install platformio
WORKDIR /esp
RUN chown 1000:1000 /esp
# ESP-IDF calls expects python to be python3
RUN ln -s /usr/bin/python3 /usr/bin/python
USER 1000
RUN git clone --recursive -b v5.1.2 https://github.com/espressif/esp-idf.git
ENV IDF_PATH="/esp/esp-idf"
RUN ./esp-idf/install.sh all
RUN ./esp-idf/tools/idf_tools.py install esp-clang
RUN ./esp-idf/tools/idf_tools.py export

USER 0
WORKDIR /app
ENTRYPOINT ["/app/builder/entrypoint.sh"]

FROM ubuntu:24.04
ARG DEBIAN_FRONTEND=noninteractive
ARG DEBCONF_NONINTERACTIVE_SEEN=true

RUN apt-get update \
    && apt-get install --no-install-recommends --yes \
    bind9-host \
    binutils \
    bison \
    build-essential \
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
    golang-1.21 \
    gperf \
    less \
    libarchive-tools \
    libdbus-1-dev \
    libffi-dev \
    libfl-dev \
    libglib2.0-dev \
    libssl-dev \
    libusb-1.0-0 \
    lldb \
    mc \
    ninja-build \
    openssl \
    patch \
    python3 \
    python3-dev \
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
    && apt-get autoremove -y

RUN update-alternatives --install /usr/bin/go go /usr/lib/go-1.21/bin/go 0
RUN update-alternatives --install /usr/bin/gofmt gofmt /usr/lib/go-1.21/bin/gofmt 0
RUN gem install --no-document fpm

WORKDIR /zig
RUN wget -O zig.tar.xz "https://ziglang.org/download/0.12.0/zig-linux-x86_64-0.12.0.tar.xz" \
    && tar -xf zig.tar.xz \
    && rm -f zig.tar.xz \
    && ln -s /zig/zig-linux-x86_64-0.12.0/zig /usr/bin/zig \
    && ln -s /zig/zig-linux-x86_64-0.12.0 /zig/current

WORKDIR /jsonnet
RUN wget -O jsonnet.tar.gz "https://github.com/google/go-jsonnet/releases/download/v0.20.0/go-jsonnet_0.20.0_Linux_x86_64.tar.gz" \
    && tar -xf jsonnet.tar.gz \
    && rm -f jsonnet.tar.gz \
    && ln -s /jsonnet/jsonnet /usr/bin/ \
    && ln -s /jsonnet/jsonnetfmt /usr/bin/

RUN userdel -r ubuntu && groupadd builder \
    && useradd -m -u 1000 -g 1000 builder \
    && echo "builder ALL=(ALL:ALL) NOPASSWD:ALL" >> /etc/sudoers

ENV CGO_ENABLED="0"
ENV GOCACHE="/app/build/gocache"
ENV GOPATH="/app/build/gopath"
ENV GOFLAGS="-buildvcs=false"

WORKDIR /esp
RUN chown 1000:1000 /esp
RUN usermod -aG dialout builder
# ESP-IDF calls expects python to be python3
RUN ln -s /usr/bin/python3 /usr/bin/python
USER 1000
RUN git clone --recursive --depth 1 --shallow-submodules -b v5.3.2 https://github.com/espressif/esp-idf.git
ENV IDF_PATH="/esp/esp-idf"
ENV IDF_TOOLS_PATH="/esp/tools"
RUN ./esp-idf/install.sh all --enable-pytest --enable-ci \
    && rm -rf $IDF_TOOLS_PATH/dist
RUN ./esp-idf/tools/idf_tools.py export

# The constraint file has been downloaded and the right Python package versions installed. No need to check and
# download this at every invocation of the container.
ENV IDF_PYTHON_CHECK_CONSTRAINTS=no

# Ccache is installed, enable it by default
ENV IDF_CCACHE_ENABLE=1

USER 0
WORKDIR /app
ENTRYPOINT ["/app/builder/entrypoint.sh"]

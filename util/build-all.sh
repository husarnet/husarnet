#!/bin/bash
source $(dirname "$0")/bash-base.sh

mkdir -p ${release_base}

# Unix builds

for arch in $unix_archs; do
    ${util_base}/build-cmake.sh $arch unix
    cp ${build_base}/${arch}/unix/husarnet-daemon ${release_base}/husarnet-daemon-unix-${arch}

    for package in $unix_packages; do
        ${util_base}/package-unix.sh $arch $package
        cp ${build_base}/${arch}/unix/husarnet-daemon-${arch}.${package} ${release_base}/husarnet-daemon-${package_version}-${arch}.${package}
    done
done

exit 0

# Docker builds

# This is commented out as it's intended to be disabled in CI. You can still run it manually.
# ${util_base}/build-docker.sh

# Windows builds

${util_base}/build-cmake.sh win64 windows
# ${util_base}/build-cmake.sh win32 windows

cp ${build_base}/win64/windows/husarnet.exe ${release_base}/

# ESP32 builds

# ${util_base}/build-esp32.sh

# Smartcard module builds

# TODO long term - add smartcard builder

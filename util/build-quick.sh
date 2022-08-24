#!/bin/bash
source $(dirname "$0")/bash-base.sh

mkdir -p ${release_base}

# Unix builds

unix_archs="amd64"

for arch in $unix_archs; do
    ${util_base}/build-cmake.sh $arch unix
    cp ${build_base}/${arch}/unix/husarnet-daemon ${release_base}/husarnet-daemon-unix-${arch}

    output_dir="${base_dir}/build/${arch}/unix/out"
    ${util_base}/build-cli.sh ${arch} unix
    cp ${build_base}/bin/husarnet-linux-${arch} ${release_base}/husarnet-unix-${arch}
    cp ${build_base}/bin/husarnet-linux-${arch} ${output_dir}/usr/bin/husarnet

    for package in $unix_packages; do
        ${util_base}/package-unix.sh $arch $package
        cp ${build_base}/${arch}/unix/husarnet-${arch}.${package} ${release_base}/husarnet-${package_version}-${arch}.${package}
        ln -s ${release_base}/husarnet-${package_version}-${arch}.${package} ${release_base}/husarnet-${arch}.${package}
    done
done

exit 0

# Docker builds

# This is commented out as it's intended to be disabled in CI. You can still run it manually.
# ${util_base}/build-docker.sh

# Windows builds

${util_base}/build-cmake.sh win64 windows
${util_base}/build-cli.sh win64 windows
# cp ${build_base}/win64/windows/husarnet.exe
cp ${build_base}/bin/husarnet-windows-amd64 ${release_base}/husarnet.exe

# ESP32 builds

# ${util_base}/build-esp32.sh

# Smartcard module builds

# TODO long term - add smartcard builder

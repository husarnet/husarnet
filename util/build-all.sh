#!/bin/bash
source $(dirname "$0")/bash-base.sh

mkdir -p ${release_base}

# Unix builds

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
    done
done

# Windows build - win64 only

${util_base}/build-cmake.sh win64 windows
cp ${build_base}/win64/windows/out/exe/husarnet-daemon.exe ${release_base}/husarnet-daemon.exe

${util_base}/build-cli.sh win64 windows
cp ${build_base}/bin/husarnet-windows-win64 ${release_base}/husarnet.exe

# ESP32 builds

# ${util_base}/build-esp32.sh

# Smartcard module builds

# TODO long term - add smartcard builder

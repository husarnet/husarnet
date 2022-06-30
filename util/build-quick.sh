#!/bin/bash
source $(dirname "$0")/bash-base.sh

mkdir -p ${release_base}

# Unix build
echo "===== Unix ====="

arch=amd64
${util_base}/build-cmake.sh $arch unix
cp ${build_base}/${arch}/unix/husarnet-daemon ${release_base}/husarnet-daemon-unix-${arch}

for package in $unix_packages; do
    ${util_base}/package-unix.sh $arch $package
    cp ${build_base}/${arch}/unix/husarnet-${arch}.${package} ${release_base}/husarnet-${package_version}-${arch}.${package}
done


echo "===== building golang cli ====="
${util_base}/build-cli.sh

exit 0

# Docker builds
# echo "===== Docker ====="

# This is commented out as it's intended to be disabled in CI. You can still run it manually.
# ${util_base}/build-docker.sh

# Windows builds
echo "===== Windows ====="

${util_base}/build-cmake.sh win64 windows
# ${util_base}/build-cmake.sh win32 windows

cp ${build_base}/win64/windows/husarnet.exe ${release_base}/

# ESP32 builds

# ${util_base}/build-esp32.sh

#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ "$#" -eq 0 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <architecture> <platform>"
    exit 1
fi

# linux/arm64/v8, linux/amd64, linux/arm/v7
# buildx convert
if [ "$#" -eq 1 ]; then
    if [ "$1" = "linux/amd64" ]; then
      arch=amd64
      platform=unix
    elif [ "$1" = "linux/arm/v7" ]; then
      arch=armhf
      platform=unix
    elif [ "$1" = "linux/arm64" ]; then
      arch=arm64
      platform=unix
    fi
fi

if [ "$#" -eq 2 ]; then
    arch=$1
    platform=$2
fi

echo "[HUSARNET BS] Building ${platform} ${arch} daemon"

build_dir="${base_dir}/build/${arch}/${platform}"
output_dir="${build_dir}/out"
source_dir="${base_dir}/daemon/${platform}"

# Prepare required directories
mkdir -p ${build_dir}
mkdir -p ${output_dir}
mkdir -p ${release_base}

# Actually build the thing
pushd ${build_dir}

cmake -G "Ninja" \
      -DCMAKE_INSTALL_PREFIX=${output_dir} \
      -DCMAKE_TOOLCHAIN_FILE=${source_dir}/arch_${arch}.cmake \
      -DBUILD_SHARED_LIBS=false \
      ${source_dir}

# If you want to include debugging symbols move it up a fair bit
#      -DCMAKE_BUILD_TYPE=Debug \

cmake --build ${build_dir}
cmake --build ${build_dir} --target install

if [ ${platform} == "unix" ]; then
  cp ${build_dir}/husarnet-daemon ${release_base}/husarnet-daemon-${platform}-${arch}
  cp ${build_dir}/husarnet-daemon ${release_base}/husarnet-daemon
elif [ ${platform} == "windows" ]; then
  cp ${build_dir}/husarnet-daemon.exe ${release_base}/husarnet-daemon-${platform}-${arch}.exe
  cp ${build_dir}/husarnet-daemon.exe ${release_base}/husarnet-daemon.exe
fi

popd

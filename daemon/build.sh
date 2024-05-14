#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

# This file is intended to be run only from inside Docker!

if [ $# -ne 3 ]; then
  echo "Usage: $0 <platform> <architecture> (stable/nightly)"
  exit 1
fi

platform=$1
arch=$2
build_type=$3

echo "[HUSARNET BS] Building ${platform} ${arch} daemon"

build_dir="${base_dir}/build/${platform}/${arch}"
output_dir="${build_dir}/out"
source_dir="${base_dir}/daemon"

# Prepare required directories
mkdir -p ${build_dir}
mkdir -p ${output_dir}
mkdir -p ${release_base}

# Actually build the thing
pushd ${build_dir}

if [[ ${build_type} = nightly ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Debug"
elif [[ ${build_type} = stable ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Release"
else
  echo "Unknown build type: ${build_type}, supported values: stable/nightly"
  exit 1
fi

cmake -G "Ninja" \
  -DCMAKE_TOOLCHAIN_FILE=${source_dir}/arch_${arch}.cmake \
  -DCMAKE_INSTALL_PREFIX=${output_dir} \
  -DBUILD_SHARED_LIBS=false \
  ${debug_flags} \
  ${source_dir}

cmake --build ${build_dir}
cmake --build ${build_dir} --target install

if [[ ${platform} == "linux" || ${platform} == "macos" ]]; then
  cp ${build_dir}/husarnet-daemon ${release_base}/husarnet-daemon-${platform}-${arch}
elif [ ${platform} == "windows" ]; then
  cp ${build_dir}/husarnet-daemon.exe ${release_base}/husarnet-daemon-${platform}-${arch}.exe
fi

popd

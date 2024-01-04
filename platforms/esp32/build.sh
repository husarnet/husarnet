#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh
if [ $# -ne 2 ]; then
  echo "Usage: $0 <idf_target> (stable/nightly)"
  exit 1
fi

TARGET=$1
build_type=$2

build_dir="${base_dir}/build/esp32"
source_dir="${base_dir}/platforms/esp32"

set +u # ESP-IDF is "slightly broken"
. /esp/esp-idf/export.sh
set -u

if [[ ${build_type} = nightly ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Debug"
elif [[ ${build_type} = stable ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Release"
else
  echo "Unknown build type: ${build_type}, supported values: stable/nightly"
  exit 1
fi

# Prepare required directories
mkdir -p ${build_dir}
cp ${source_dir}/sdkconfig ${build_dir}/sdkconfig
cp ${source_dir}/partitions.csv ${build_dir}/partitions.csv

pushd ${build_dir}

# Build husarnet_core with GCC compiler
# TODO: when the clang will be officially supported by ESP-IDF, switch to it
cmake ${source_dir} \
  -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-${TARGET}.cmake \
  -DIDF_TARGET=${TARGET} \
  -DSDKCONFIG=${build_dir}/sdkconfig \
  -GNinja ${debug_flags} \

cmake --build ${build_dir}

# cp build/libhusarnet_core.a demo/
# pushd demo
# pio run
# popd

popd

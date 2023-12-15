#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh
if [ $# -ne 1 ]; then
  echo "Usage: $0 (stable/nightly)"
  exit 1
fi

build_type=$1

build_dir="${base_dir}/build/esp32/"
source_dir="${base_dir}/platforms/esp32/"

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
cp ${source_dir}/sdkconfig ${build_dir}

pushd ${build_dir}

TARGET=esp32
cmake ${source_dir} \
  -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-clang-${TARGET}.cmake \
  -DTARGET=${TARGET} \
  -DSDKCONFIG=${build_dir}/sdkconfig \
  -GNinja ${debug_flags} \

cmake --build ${build_dir}

# cp build/libhusarnet_core.a demo/
# pushd demo
# pio run
# popd

popd

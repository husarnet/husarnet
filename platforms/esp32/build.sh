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

build_dir="${base_dir}/build/${TARGET}"
source_dir="${base_dir}/platforms/esp32"

# Initialize ESP-IDF environment
. /esp/esp-idf/export.sh

# TODO: select sdkconfig.defaults file based on the build configuration
if [[ ${build_type} = nightly ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Debug"
elif [[ ${build_type} = stable ]]; then
  debug_flags="-DCMAKE_BUILD_TYPE=Release"
else
  echo "Unknown build type: ${build_type}, supported values: stable/nightly"
  exit 1
fi

pushd ${base_dir}

# Build husarnet_core with GCC compiler
# TODO: when the clang will be officially supported by ESP-IDF, switch to it
idf.py -B ${build_dir} -C ${source_dir} set-target ${TARGET}
idf.py -B ${build_dir} -C ${source_dir} build

popd

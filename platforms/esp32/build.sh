#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

echo "[HUSARNET BS] Building ESP32 library"

echo "[HUSARNET BS] Building ESP32 library"

if [ $# -ne 2 ]; then
  echo "Usage: $0 <idf_target>"
  exit 1
fi

TARGET=$1

build_dir="${base_dir}/build/${TARGET}"
source_dir="${base_dir}/platforms/esp32"

# Initialize ESP-IDF environment
. /esp/esp-idf/export.sh

pushd ${base_dir}

# Build husarnet_core with GCC compiler
# TODO: when the clang will be officially supported by ESP-IDF, switch to it
idf.py -B ${build_dir} -C ${source_dir} set-target ${TARGET}
idf.py -B ${build_dir} -C ${source_dir} build

popd

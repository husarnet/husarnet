#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

echo "[HUSARNET BS] Building ESP32 library"

pushd ${base_dir}/platforms/esp32

set +u # ESP-IDF is "slightly broken"
. /esp/esp-idf/export.sh
set -u

rm -rf build && mkdir -p build
pushd build

TARGET=esp32
cmake .. -DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-${TARGET}.cmake -DTARGET=${TARGET} -GNinja
cmake --build .

popd

cp build/libhusarnet_core.a demo/
pushd demo
pio run
popd

popd

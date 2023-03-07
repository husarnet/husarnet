#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

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

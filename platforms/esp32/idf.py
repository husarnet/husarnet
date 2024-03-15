#!/bin/bash

# TODO: set target
TARGET="esp32s3"
idf.py -B build/esp32 -C platforms/esp32/ -DCMAKE_TOOLCHAIN_FILE="${IDF_PATH}"/tools/cmake/toolchain-"${TARGET}".cmake "$@"
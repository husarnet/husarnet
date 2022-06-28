#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${base_dir}

go() {
f=$1
SRC="^#include \"${f}\""
DST="#include \"husarnet/${f}\""

find . -regextype posix-egrep -regex ".*\.(cpp|hpp|h|c)" -not -path "./build/*" -exec sed -i "s%${SRC}%${DST}%g" {} \;
}

go "config_manager.h"

popd

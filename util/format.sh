#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${base_dir}
find . -regextype posix-egrep -regex ".*\.(cpp|hpp|h|c)" -not -path "./build/*" -exec clang-format-14 -style=file -i {} \;
popd

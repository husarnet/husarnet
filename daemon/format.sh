#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

format() {
    suffix=$1
    echo "[HUSARNET BS] clang-formatting ${suffix}"
    pushd ${base_dir}/$suffix
    find . -regextype posix-egrep -regex ".*\.(cpp|hpp|h|c)" -exec clang-format -style=file -i {} \;
    popd
}

format core
format daemon
format tests/unit

if [ ${in_ci} != false ]; then
    git diff # to print changes to logs

    if [ $(git diff | wc -l) -gt 0 ]; then
        echo Found differences!
        exit 1
    else
        exit 0
    fi
fi

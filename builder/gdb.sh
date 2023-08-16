#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

if [ $# -lt 1 ]; then
    echo "Usage: $0 <path-to-executable>"
    exit 1
fi

pushd ${base_dir}/builder
./shell.sh gdb -batch -ex "catch throw" -ex "run" -ex "bt full" -ex "quit" --args /app/$1
popd

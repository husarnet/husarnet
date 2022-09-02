#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${base_dir}
find . -regextype posix-egrep -regex ".*\.(cpp|hpp|h|c)" -not -path "./build/*" -exec clang-format-14 -style=file -i {} \;

git diff # to print changes to logs

popd

if [ $(git diff | wc -l) -gt 0 ]; then
    echo Found differences!
    exit 1
else
    exit 0
fi

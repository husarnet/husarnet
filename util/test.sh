#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${base_dir}

docker compose -f builder/compose.yml up --exit-code-from test test

popd

#!/bin/bash
source $(dirname "$0")/bash-base.sh

pushd ${base_dir}

docker compose -f builder/compose.yml run test

popd

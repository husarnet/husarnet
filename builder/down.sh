#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}
docker compose -f builder/compose.yml down
popd

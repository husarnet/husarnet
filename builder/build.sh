#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Building the builder"

pushd ${base_dir}
docker compose -f builder/compose.yml build builder
popd

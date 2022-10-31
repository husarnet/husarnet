#!/bin/bash
source $(dirname "$0")/bash-base.sh

echo "[HUSARNET BS] Updating GitHub workflows"

pushd ${base_dir}
docker compose -f builder/compose.yml up --exit-code-from gh gh
popd

echo "To inspect the image manually run:"
echo './builder/run.sh'

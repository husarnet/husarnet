#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Running cppcheck"

# TODO enable cppcheck in the next commit (fixing found issues)
# cppcheck --error-exitcode=1 --enable=all ${base_dir}/daemon ${base_dir}/core

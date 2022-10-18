#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

output_dir="${build_base}/tests"

mkdir -p $output_dir
pushd $output_dir

echo "[HUSARNET BS] Building unit tests"
cmake -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=${base_dir}/daemon/arch_amd64.cmake \
      -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_SHARED_LIBS=false \
      ${tests_base}/unit

cmake --build ${output_dir}

echo "[HUSARNET BS] Running unit tests"
${output_dir}/husarnet_tests

popd

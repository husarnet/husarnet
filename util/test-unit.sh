#!/bin/bash
source $(dirname "$0")/bash-base.sh

output_dir="${build_tests_base}/out"

mkdir -p ${build_tests_base}

rm -fr ${output_dir}
mkdir -p ${output_dir}

pushd ${build_tests_base}

cmake -G Ninja \
      -DCMAKE_INSTALL_PREFIX=${output_dir} \
      -DCMAKE_TOOLCHAIN_FILE=${base_dir}/daemon/unix/arch_amd64.cmake \
      -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_SHARED_LIBS=false \
      ${tests_base}

cmake --build ${build_tests_base}

./husarnet_tests

popd

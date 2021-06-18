#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <architecture> <platform>"
    exit 1
fi

arch=$1
platform=$2
build_dir="${base_dir}/build/${arch}/${platform}"
output_dir="${build_dir}/out"
source_dir="${base_dir}/${platform}"

# Prepare required directories
mkdir -p ${build_dir}

rm -fr ${output_dir}
mkdir -p ${output_dir}

# Actually build the thing
pushd ${build_dir}

cmake -G Ninja \
      -DSODIUM_SRC=${base_dir}/deps/libsodium/src/libsodium \
      -DCMAKE_INSTALL_PREFIX=${output_dir} \
      -DCMAKE_TOOLCHAIN_FILE=${source_dir}/arch_${arch}.cmake \
      -DBUILD_SHARED_LIBS=false \
      ${source_dir}

# If you want to include debugging symbols move it up a fair bit
#      -DCMAKE_BUILD_TYPE=Debug \

cmake --build ${build_dir}

cmake --build ${build_dir} --target install

popd

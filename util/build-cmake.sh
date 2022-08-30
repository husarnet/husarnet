#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ "$#" -eq 0 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <architecture> <platform>"
    exit 1
fi

# linux/arm64/v8, linux/amd64, linux/arm/v7
# buildx convert
if [ "$#" -eq 1 ]; then
    if [ "$1" = "linux/amd64" ]; then
      arch=amd64
      platform=unix
    elif [ "$1" = "linux/arm/v7" ]; then
      arch=armhf
      platform=unix
    elif [ "$1" = "linux/arm64" ]; then
      arch=arm64
      platform=unix
    fi
fi

if [ "$#" -eq 2 ]; then
    arch=$1
    platform=$2
fi

build_dir="${base_dir}/build/${arch}/${platform}"
output_dir="${build_dir}/out"
source_dir="${base_dir}/daemon/${platform}"
bin_dir="${base_dir}/build/bin"

# Prepare required directories
mkdir -p ${build_dir}
mkdir -p ${bin_dir}

rm -fr ${output_dir}
mkdir -p ${output_dir}

# Actually build the thing
pushd ${build_dir}

cmake -G "Ninja" \
      -DCMAKE_INSTALL_PREFIX=${output_dir} \
      -DCMAKE_TOOLCHAIN_FILE=${source_dir}/arch_${arch}.cmake \
      -DBUILD_SHARED_LIBS=false \
      ${source_dir}


# If you want to include debugging symbols move it up a fair bit
#      -DCMAKE_BUILD_TYPE=Debug \

cmake --build ${build_dir}

cmake --build ${build_dir} --target install

# this check is because of win64, the file wont be there while building win64
if [[ -f "${output_dir}/usr/bin/husarnet-daemon" ]]; then
  cp ${output_dir}/usr/bin/husarnet-daemon ${bin_dir}/husarnet-daemon
fi

popd

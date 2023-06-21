#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

if [ ! "$#" -eq 2 ]; then
    echo "Usage: $0 <architecture> [stable/nightly]"
    exit 1
fi

platform=macos
arch="$1"
build_type=$2

platform_base=${base_dir}/platforms/${platform}

${base_dir}/daemon/build.sh ${platform} ${platform}_${arch} ${build_type}
${base_dir}/cli/build.sh ${platform} ${arch}

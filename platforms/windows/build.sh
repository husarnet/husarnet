#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh
if [ ! "$#" -eq 1 ]; then
    echo "Usage: $0 [stable/nightly]"
    exit 1
fi
build_type=$1
platform=windows
arch=win64
platform_base=${base_dir}/platforms/${platform}

${base_dir}/daemon/build.sh ${platform} ${arch} ${build_type}
${base_dir}/cli/build.sh ${platform} ${arch}

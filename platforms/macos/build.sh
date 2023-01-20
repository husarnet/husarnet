#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

if [ ! "$#" -eq 1 ]; then
    echo "Usage: $0 <architecture>"
    exit 1
fi

# Right now we only support Apple Silicon i.e. 64-bit ARM
platform=macos
arch=arm64

platform_base=${base_dir}/platforms/${platform}

${base_dir}/daemon/build.sh ${platform} ${arch}
${base_dir}/cli/build.sh ${platform} ${arch}

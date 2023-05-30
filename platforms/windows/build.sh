#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

platform=windows
arch=win64
build_type=stable
platform_base=${base_dir}/platforms/${platform}

${base_dir}/daemon/build.sh ${platform} ${arch} ${build_type}
${base_dir}/cli/build.sh ${platform} ${arch}

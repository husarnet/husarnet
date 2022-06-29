#!/bin/bash
source $(dirname "$0")/bash-base.sh

source_dir="${base_dir}/cli"
output_dir="${base_dir}/build/bin/cli"

mkdir -p ${source_dir}
mkdir -p ${output_dir}

pushd ${source_dir}

go generate
go build -o ${output_dir}/husarnet .

popd

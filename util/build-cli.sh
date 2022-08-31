#!/bin/bash
source $(dirname "$0")/bash-base.sh

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <architecture> <platform>"
    exit 1
fi

arch=$1
# we don't actually use this information right now, but
# in the future we might
platform=$2

# directory setup
source_dir="${base_dir}/cli"
output_dir="${base_dir}/build/release"

# GOOS and GOARCH setup
go_variables_suffix=""
if [ $arch = "amd64" ]; then
  goos="linux"
  goarch="amd64"
  arch_alias="unix"
elif [ $arch = "i386" ]; then
  goos="linux"
  goarch="386"
  arch_alias="unix"
elif [ $arch = "arm64" ]; then
  goos="linux"
  goarch="arm64"
  arch_alias="unix"
elif [ $arch = "armhf" ]; then
  goos="linux"
  goarch="arm"
  go_variables_suffix="GOARM=7"
  arch_alias="unix"
elif [ $arch = "riscv64" ]; then
  goos="linux"
  goarch="riscv64"
  arch_alias="unix"
elif [ $arch = "win64" ]; then
  goos="windows"
  goarch="amd64"
  arch_alias="windows"
else
  echo "Unknown architecture"
  exit 1
fi

go_variables="GOOS=${goos} GOARCH=${goarch} ${go_variables_suffix}"

# let's build this
mkdir -p ${output_dir}
pushd ${source_dir}

go generate
env ${go_variables} go build -o ${output_dir}/husarnet-${arch_alias}-${arch} .

popd

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
output_dir="${base_dir}/build/${arch}/${platform}"
go_output=${output_dir}/husarnet-${platform}-${arch}

# GOOS and GOARCH setup
go_variables_suffix=""
if [ $arch = "amd64" ]; then
  goos="linux"
  goarch="amd64"
  platform="unix"
elif [ $arch = "i386" ]; then
  goos="linux"
  goarch="386"
  platform="unix"
elif [ $arch = "arm64" ]; then
  goos="linux"
  goarch="arm64"
  platform="unix"
elif [ $arch = "armhf" ]; then
  goos="linux"
  goarch="arm"
  go_variables_suffix="GOARM=7"
  platform="unix"
elif [ $arch = "riscv64" ]; then
  goos="linux"
  goarch="riscv64"
  platform="unix"
elif [ $arch = "win64" ]; then
  goos="windows"
  goarch="amd64"
  platform="windows"
else
  echo "Unknown architecture"
  exit 1
fi

go_variables="GOOS=${goos} GOARCH=${goarch} ${go_variables_suffix}"

# let's build this
pushd ${source_dir}

echo "[HUSARNET BS] Building ${platform} ${arch} CLI"

go generate
env ${go_variables} go build -o ${go_output} .

mkdir -p ${release_base}

if [ ${platform} == "unix" ]; then
  mkdir -p ${output_dir}/out/usr/bin
  cp ${go_output} ${output_dir}/out/usr/bin/husarnet
  cp ${go_output} ${release_base}/husarnet-${platform}-${arch}
elif [ ${platform} == "windows" ]; then
  cp ${go_output} ${release_base}/husarnet-${platform}-${arch}.exe
fi

popd

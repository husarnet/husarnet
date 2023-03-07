#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <platform> <architecture>"
    exit 1
fi

platform=$1
arch=$2

# directory setup
source_dir="${base_dir}/cli"
if [[ ${platform} == "unix" || ${platform} == "macos" ]]; then
  go_output=${release_base}/husarnet-${platform}-${arch}
elif [ ${platform} == "windows" ]; then
  go_output=${release_base}/husarnet-${platform}-${arch}.exe
fi

# GOOS and GOARCH setup
go_variables_suffix=""
if [ $platform == "macos" ]; then
  goos="darwin"
  goarch="arm64"
elif [ $arch = "amd64" ]; then
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

popd

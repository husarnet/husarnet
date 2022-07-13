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
output_dir="${base_dir}/build/bin"

mkdir -p ${output_dir}

pushd ${source_dir}

# GOOS and GOARCH setup
goos="linux"
goarch="amd64"
if [ $arch = "i386" ]
then
  goarch="386"
  go_variables="GOOS=${goos} GOARCH=${goarch}"
elif [ $arch = "armhf" ]
then
  goarch="arm"
  go_variables="GOOS=${goos} GOARCH=${goarch} GOARM=7"
elif [ $arch = "win64" ]
then
  goos="windows"
  goarch="amd64"
  go_variables="GOOS=${goos} GOARCH=${goarch}"
else
  go_variables="GOOS=${goos} GOARCH=${arch}"
fi

# let's build this
go generate
env ${go_variables} go build -o ${output_dir}/husarnet-${goos}-${arch} .

popd

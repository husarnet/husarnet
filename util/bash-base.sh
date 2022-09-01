#!/bin/bash
set -euo pipefail

catch() {
  if [ "$1" != "0" ]; then
    echo "$0, line $2: Error $1"
  fi
}
trap 'catch $? $LINENO' EXIT

pushd () {
    builtin pushd "$@" > /dev/null
}

popd () {
    builtin popd "$@" > /dev/null
}

base_dir="$(realpath $(dirname "$0")/..)"
util_base="$(realpath $(dirname "$0"))"
tests_base="$(realpath $(dirname "$0")/../tests)"
build_base="$(realpath $(dirname "$0")/../build)"
release_base="${build_base}/release"
build_tests_base="${build_base}/tests"
deploy_base="$(realpath $(dirname "$0")/../deploy)"

function update_version {
  package_version="$(cat ${base_dir}/core/husarnet_config.h | grep -P '#define HUSARNET_VERSION "\d+.\d+.\d+.\d+"' | cut -d'"' -f2)"
}

update_version

unix_archs="amd64 i386 arm64 armhf riscv64"
unix_packages="tar deb rpm"

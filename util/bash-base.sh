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

util_base="$(realpath $(dirname "$BASH_SOURCE[0]"))"
base_dir="${util_base}/.."
tests_base="${base_dir}/tests"
build_base="${base_dir}/build"
release_base="${build_base}/release"
deploy_base="${base_dir}/deploy"
builder_base="${base_dir}/builder"

# Version checking
# This is done this way so we can update it mid-script and change globally
function update_version {
  package_version="$(cat ${base_dir}/version.txt)"
}

update_version

in_ci=${CI:-false}

function docker_builder() {
  docker run --rm -it --privileged --volume ${base_dir}:/app ghcr.io/husarnet/husarnet:builder ${@}
}

#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
set -uo pipefail

catch() {
  # In ERR catch this seems redundant
  if [ "$1" == "0" ]; then
    return
  fi

  local i
  echo "Error $1 happened. Stack trace:"

  # This should never be true but I still don't trust it
  if [ ${FUNCNAME[0]} != "catch" ]; then
    echo "$(realpath ${BASH_SOURCE[0]}):${LINENO} in ${FUNCNAME[0]}"
  fi

  for ((i = 1; i < ${#FUNCNAME[*]}; i++)); do
    echo "$(realpath ${BASH_SOURCE[$i]}):${BASH_LINENO[$i - 1]} in ${FUNCNAME[$i]}"
  done

  # Propagate the same exit status as before
  exit $1
}
trap 'catch $?' ERR

pushd() {
  builtin pushd "$@" >/dev/null
}

popd() {
  builtin popd "$@" >/dev/null
}

util_base="$(realpath $(dirname "$BASH_SOURCE[0]"))"
base_dir="${util_base}/.."
tests_base="${base_dir}/tests"
build_base="${base_dir}/build"
release_base="${build_base}/release"
deploy_base="${base_dir}/deploy"
builder_base="${base_dir}/builder"
platforms_base="${base_dir}/platforms"

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

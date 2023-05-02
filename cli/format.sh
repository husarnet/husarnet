#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}/cli
gofmt -w .
popd

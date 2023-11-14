#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

echo "[HUSARNET BS] Building the GitHub workflows"

# Please note that this script is meant to be run from inside of an environment
# with jsonnet installed (i.e. the builder container).
# You'll find an accompanying script in `util` for "human" use.

pushd ${base_dir}

sources_dir=${base_dir}/.github/workflow_sources
dst_dir=${base_dir}/.github/workflows

jsonnetfmt -i ${sources_dir}/*.libsonnet

regenerate_json() {
    base=$(basename ${1})
    src_name=${base%.*}

    echo ${src_name}
    jsonnetfmt -i ${sources_dir}/${src_name}.jsonnet
    jsonnet -J ${sources_dir} -S -o ${dst_dir}/${src_name}.yml ${sources_dir}/${src_name}.jsonnet
}

for filename in $(ls ${sources_dir}/*.jsonnet); do
    regenerate_json $filename
done

popd

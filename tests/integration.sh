#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

# We'll need to decrypt the secrets first
${tests_base}/secrets-tool.sh decrypt

pushd ${base_dir}

# This is only a list for local-ci (hence a slightly shorter list)
# GH actions integration tests are defined in .github/workflow_sources/common.jsonnet
parallel --progress --eta --halt soon,fail=1 docker run --rm --privileged --tmpfs /var/lib/husarnet:rw,exec --volume "${base_dir}:/app" {1} /app/tests/integration/runner-wrapper.sh {2} {3} {1} \
    ::: \
    husarnet:dev \
    ubuntu:18.04 ubuntu:22.04 ubuntu:24.04 \
    debian:stable \
    fedora:41 \
    :::+ \
    docker \
    ubuntu ubuntu ubuntu \
    debian \
    fedora \
    ::: \
    functional-basic join-workflow hooks-basic hooks-rw

popd

#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

# We'll need to decrypt the secrets first
${tests_base}/integration/secrets-tool.sh decrypt

pushd ${base_dir}

parallel --progress --eta --halt soon,fail=1 docker run --rm --privileged --tmpfs /var/lib/husarnet:rw,exec --volume ${base_dir}:/app {1} /app/tests/integration/runner-wrapper.sh {2} {3} {1} \
    ::: \
    husarnet:dev \
    ubuntu:18.04 ubuntu:20.04 ubuntu:22.04 ubuntu:24.04 \
    debian:oldstable debian:stable debian:testing \
    fedora:38 fedora:39 fedora:40 fedora:41 \
    :::+ \
    docker \
    ubuntu ubuntu ubuntu ubuntu \
    debian debian debian \
    fedora fedora fedora fedora \
    ::: \
    functional-basic join-workflow hooks-basic hooks-rw

# Remember to keep those ^ in sync with GH actions!

popd

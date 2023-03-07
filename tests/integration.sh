#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}

# I'm terribly sorry for that part of code but it was a Monday morning…
# --color-failed if you have a sufficiently new parallel version
parallel docker run --rm --privileged --volume ${base_dir}:/app {1} /app/tests/integration/runner.sh {2} {3} \
    ::: husarnet:dev \
    ubuntu:18.04 ubuntu:20.04 ubuntu:22.04 \
    debian:oldstable debian:stable debian:testing \
    fedora:37 fedora:38 \
    :::+ docker \
    ubuntu_debian ubuntu_debian ubuntu_debian \
    ubuntu_debian ubuntu_debian ubuntu_debian \
    fedora fedora fedora \
    ::: functional-basic join-workflow

popd

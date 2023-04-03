#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

# We'll need to decrypt the secrets first
${tests_base}/integration/secrets-tool.sh decrypt

pushd ${base_dir}

echo "Test the test engine itself I guess"
docker run --rm --privileged --volume ${base_dir}:/app ubuntu:22.04 /app/tests/integration/runner.sh ubuntu functional-basic

if [ $(man parallel | grep color-failed | wc -l) -gt 1 ]; then
    parallel_cmd="parallel --color-failed"
else
    parallel_cmd="parallel"
fi

$parallel_cmd docker run --rm --privileged --volume ${base_dir}:/app {1} /app/tests/integration/runner.sh {2} {3} \
    ::: husarnet:dev \
    ubuntu:18.04 ubuntu:20.04 ubuntu:22.04 \
    debian:oldstable debian:stable debian:testing \
    fedora:37 fedora:38 \
    :::+ docker \
    ubuntu ubuntu ubuntu \
    debian debian debian \
    fedora fedora fedora \
    ::: functional-basic join-workflow

popd

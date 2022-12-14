#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}

${builder_base}/down.sh

docker compose -f builder/compose.yml up --exit-code-from test_docker test_docker
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_18_04 test_ubuntu_18_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_20_04 test_ubuntu_20_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_22_04 test_ubuntu_22_04
# docker compose -f builder/compose.yml up --exit-code-from test_fedora_35 test_fedora_35
# docker compose -f builder/compose.yml up --exit-code-from test_fedora_36 test_fedora_36
# docker compose -f builder/compose.yml up --exit-code-from test_fedora_37 test_fedora_37
docker compose -f builder/compose.yml up --exit-code-from test_fedora_38 test_fedora_38

popd

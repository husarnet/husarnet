#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}

${builder_base}/down.sh

docker compose -f builder/compose.yml up --exit-code-from test_docker test_docker
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_18_04 test_ubuntu_18_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_20_04 test_ubuntu_20_04
docker compose -f builder/compose.yml up --exit-code-from test_ubuntu_22_04 test_ubuntu_22_04
docker compose -f builder/compose.yml up --exit-code-from test_debian_oldstable test_debian_oldstable
docker compose -f builder/compose.yml up --exit-code-from test_debian_stable test_debian_stable
docker compose -f builder/compose.yml up --exit-code-from test_debian_testing test_debian_testing
docker compose -f builder/compose.yml up --exit-code-from test_fedora_37 test_fedora_37
docker compose -f builder/compose.yml up --exit-code-from test_fedora_38 test_fedora_38

popd

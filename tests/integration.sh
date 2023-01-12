#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

pushd ${base_dir}

${builder_base}/down.sh

${tests_base}/integration/secrets-encrypt.sh

docker compose -f builder/compose.yml run test_docker
docker compose -f builder/compose.yml run test_ubuntu_18_04
docker compose -f builder/compose.yml run test_ubuntu_20_04
docker compose -f builder/compose.yml run test_ubuntu_22_04
docker compose -f builder/compose.yml run test_debian_oldstable
docker compose -f builder/compose.yml run test_debian_stable
docker compose -f builder/compose.yml run test_debian_testing
docker compose -f builder/compose.yml run test_fedora_37
docker compose -f builder/compose.yml run test_fedora_38

popd

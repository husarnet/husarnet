#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

mkdir -p ${build_base}
chown 1000:1000 ${build_base}
chmod 777 ${build_base}

su - builder bash -c "$*"

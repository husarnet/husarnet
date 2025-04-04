#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../util/bash-base.sh

fix_perms() {
    mkdir -p $1
    chown -R 1000:1000 $1
    chmod 777 $1
}

fix_perms ${base_dir}/build
fix_perms ${base_dir}/cli/generated
fix_perms ${base_dir}/tests
fix_perms ${base_dir}/platforms/esp32

git config --global --add safe.directory ${build_base}

su -w "CGO_ENABLED,GOCACHE,GOPATH,GOFLAGS,IDF_PATH,IDF_TOOLS_PATH,IDF_PYTHON_CHECK_CONSTRAINTS" -l -P builder -c "/usr/bin/bash ${*}"

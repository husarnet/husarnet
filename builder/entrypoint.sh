#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

fix_perms () {
    mkdir -p $1
    chown 1000:1000 $1
    chmod 777 $1
}

fix_perms ${build_base}
fix_perms ${base_dir}/cli/generated

su - -w CGO_ENABLED,GOCACHE,GOFLAGS builder bash -c "$*"

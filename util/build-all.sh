#!/bin/bash
source $(dirname "$0")/bash-base.sh

is_quick=${QUICK:-}

build_cli=1
build_daemon=1

build_windows=1
build_docker=1

if [ $is_quick ]; then
    unix_archs="amd64"
    build_docker=""
    # unix_packages=""
    # build_windows= # if you want to disable something - make the variable empty
fi

# Unix builds

for arch in $unix_archs; do
    if [ ${build_daemon} ]; then
        ${util_base}/build-cmake.sh $arch unix
    fi

    if [ ${build_cli} ]; then
        ${util_base}/build-cli.sh ${arch} unix
    fi

    for package in $unix_packages; do
        ${util_base}/package-unix.sh $arch $package
    done
done

# Windows build - win64 only

if [ ${build_windows} ] && [ ${build_daemon} ]; then
    ${util_base}/build-cmake.sh win64 windows
fi

if [ ${build_windows} ] && [ ${build_cli} ]; then
    ${util_base}/build-cli.sh win64 windows
fi

if [ ${build_docker} ]; then
    ${util_base}/build-docker.sh
fi

# ESP32 builds

# ${util_base}/build-esp32.sh

# Smartcard module builds

# TODO long term - add smartcard builder

echo "[HUSARNET BS] All done!"

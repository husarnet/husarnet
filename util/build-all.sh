#!/bin/bash
source $(dirname "$0")/bash-base.sh

mkdir -p ${release_base}

is_quick=${QUICK:-}

build_cli=1
build_daemon=1

build_windows=1

if [ $is_quick ]; then
    unix_archs="amd64"
    unix_packages=""
    build_windows= # if you want to disable something - make the variable empty
fi

# Unix builds

for arch in $unix_archs; do
    output_dir="${base_dir}/build/${arch}/unix/out"

    if [ ${build_daemon} ]; then
        echo "[HUSARNET BS] Building unix ${arch} daemon"
        ${util_base}/build-cmake.sh $arch unix
        cp ${build_base}/${arch}/unix/husarnet-daemon ${release_base}/husarnet-daemon-unix-${arch}
    fi

    if [ ${build_cli} ]; then
        echo "[HUSARNET BS] Building unix ${arch} CLI"
        ${util_base}/build-cli.sh ${arch} unix
        cp ${release_base}/husarnet-unix-${arch} ${output_dir}/usr/bin/husarnet
    fi

    for package in $unix_packages; do
        echo "[HUSARNET BS] Building unix ${arch} ${package} package"
        ${util_base}/package-unix.sh $arch $package
        cp ${build_base}/${arch}/unix/husarnet-${arch}.${package} ${release_base}/husarnet-${package_version}-${arch}.${package}
        ln -fs ${release_base}/husarnet-${package_version}-${arch}.${package} ${release_base}/husarnet-${arch}.${package}
    done
done

# Windows build - win64 only

if [ ${build_windows} ] && [ ${build_daemon} ]; then
    echo "[HUSARNET BS] Building windows daemon"
    ${util_base}/build-cmake.sh win64 windows
    cp ${build_base}/win64/windows/out/exe/husarnet-daemon.exe ${release_base}/husarnet-daemon.exe
fi

if [ ${build_windows} ] && [ ${build_cli} ]; then
    echo "[HUSARNET BS] Building windows CLI"
    ${util_base}/build-cli.sh win64 windows
    cp ${build_base}/bin/husarnet-windows-win64 ${release_base}/husarnet.exe
fi

# ESP32 builds

# ${util_base}/build-esp32.sh

# Smartcard module builds

# TODO long term - add smartcard builder

echo "[HUSARNET BS] All done!"

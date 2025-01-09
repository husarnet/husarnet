#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

# This file is intended to be run only from inside Docker!

if [ ! "$#" -eq 2 ]; then
    echo "Usage: $0 <architecture> [stable/nightly]"
    exit 1
fi

platform=linux
arch=$1
build_type=$2
platform_base=${base_dir}/platforms/${platform}

echo "[HUSARNET BS] Building Husarnet linux/${arch}"

${base_dir}/daemon/build.sh ${platform} ${arch} ${build_type}
${base_dir}/cli/build.sh ${platform} ${arch}

# Package
package_tmp=${build_base}/${platform}/${arch}/package
mkdir -p ${package_tmp}

mkdir -p ${package_tmp}/usr/share/bash-completion/completions
cp ${platform_base}/packaging/autocomplete ${package_tmp}/usr/share/bash-completion/completions/husarnet

mkdir -p ${package_tmp}/usr/bin
cp ${release_base}/husarnet-daemon-${platform}-${arch} ${package_tmp}/usr/bin/husarnet-daemon
cp ${release_base}/husarnet-${platform}-${arch} ${package_tmp}/usr/bin/husarnet
chmod -R 755 ${package_tmp}
for package_type in tar deb rpm pacman; do
    echo "[HUSARNET BS] Building ${platform} ${arch} ${package_type} package"
    if [[ "${package_type}" == 'pacman' ]]; then
        if [[ ${arch} == "armhf" ]]; then
            declared_arch="armv7h"
        elif [[ ${arch} == "arm64" ]]; then
            declared_arch="aarch64"
        elif [[ ${arch} == "amd64" ]]; then
            declared_arch="x86_64"
        else
            declared_arch=${arch}
        fi
    else
        declared_arch=${arch}
    fi

    fpm \
        --input-type dir \
        --output-type ${package_type} \
        --name husarnet \
        --version ${package_version} \
        --epoch 1 \
        --architecture ${declared_arch} \
        --maintainer "Husarnet <support@husarnet.com>" \
        --vendor Husarnet \
        --description "Global LAN network" \
        --url "https://husarnet.com" \
        $(if [ "${package_type}" == "deb" ]; then echo "--depends iproute2 --depends procps --deb-recommends sudo --deb-recommends systemd --deb-recommends fonts-noto-color-emoji"; fi) \
        $(if [ "${package_type}" == "rpm" ]; then echo "--depends iproute --depends procps-ng"; fi) \
        $(if [ "${package_type}" == "pacman" ]; then echo "--pacman-compression none --depends iproute2 --depends procps-ng --pacman-optional-depends sudo --pacman-optional-depends systemd --pacman-optional-depends noto-fonts-emoji --pacman-use-file-permissions"; fi) \
        --replaces "husarnet-ros" \
        --after-install ${platform_base}/packaging/post-install-script.sh \
        --before-remove ${platform_base}/packaging/pre-remove-script.sh \
        --package ${release_base}/husarnet-${platform}-${arch}.${package_type} \
        --force \
        --chdir ${package_tmp}

    if [[ "${package_type}" == "pacman" ]]; then
        mv ${release_base}/husarnet-${platform}-${arch}.pacman ${release_base}/husarnet-${platform}-${arch}.pkg
    fi
done

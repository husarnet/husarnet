#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

# I feel bad writing this script, but some things **really** do **not** belong inside Docker

# Fail if not root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

# Fail if not "Ubuntu" in uname -a
if [ "$(uname -a | grep -o Ubuntu)" != "Ubuntu" ]; then
    echo "This script is intended for Ubuntu"
    exit 2
fi

# Start with random tools
apt-get -y install \
    openssl \
    parallel \
    tio

# https://docs.makedeb.org/prebuilt-mpr/getting-started/#setting-up-the-repository
wget -qO - 'https://proget.makedeb.org/debian-feeds/prebuilt-mpr.pub' | gpg --dearmor >/usr/share/keyrings/prebuilt-mpr-archive-keyring.gpg
echo "deb [arch=all,$(dpkg --print-architecture) signed-by=/usr/share/keyrings/prebuilt-mpr-archive-keyring.gpg] https://proget.makedeb.org prebuilt-mpr $(lsb_release -cs)" >/etc/apt/sources.list.d/prebuilt-mpr.list
apt-get update

# https://github.com/casey/just?tab=readme-ov-file#packages
apt-get -y install just

# https://docs.docker.com/engine/install/ubuntu/
for pkg in docker.io docker-doc docker-compose docker-compose-v2 podman-docker containerd runc; do
    apt-get -y remove $pkg
done

apt-get -y install ca-certificates curl
install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
chmod a+r /etc/apt/keyrings/docker.asc

echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
  $(. /etc/os-release && echo "$VERSION_CODENAME") stable" >/etc/apt/sources.list.d/docker.list
apt-get update
apt-get -y install docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

# https://snapcraft.io/docs/installing-snap-on-ubuntu
apt-get -y install snapd

# https://snapcraft.io/docs/create-a-new-snap#h-1-snapcraft-setup-2
snap install snapcraft --classic
snap install lxd
usermod -a -G lxd $USER
lxd init --minimal

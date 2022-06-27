#!/bin/bash
source $(dirname "$0")/bash-base.sh

sudo apt update

# Initialize submodules
sudo apt install -y git
# Skip this while building CI's golden image
git submodule update --init --recursive

# Install stuff for cross building
sudo apt install -y ninja-build cmake linux-headers-generic build-essential libcap-dev crossbuild-essential-i386 crossbuild-essential-amd64 crossbuild-essential-arm64 crossbuild-essential-armhf crossbuild-essential-riscv64 g++-mingw-w64

# This is unfortunate but... (see https://stackoverflow.com/a/38751292 )
sudo ln -sf /usr/include/asm-generic /usr/include/asm

# Install stuff for packaging
sudo apt install -y ruby ruby-dev rubygems rpm
sudo gem install --no-document fpm

# Install Docker dependencies
# TODO long term - add proper repositories for docker-ce
# sudo apt install -y docker-ce qemu-user-static

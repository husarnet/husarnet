#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

# Usage: ./build.sh [platform] [architecture] [build_type]
# Note: All arguments are optional

# Check if the platform argument is provided, default to "all" if it's not
platform=${1:-all}

# Check if the architecture is provided, default to amd64 if it's not
architecture=${2:-amd64}

# Check if the build type is provided, default to nightly if it's not
build_type=${3:-nightly}

# Linux
if [ "$platform" == "all" ] || [ "$platform" == "linux" ] || [ "$platform" == "docker" ] || [ "$platform" == "snap" ]; then
    docker_builder /app/platforms/linux/build.sh ${architecture} ${build_type}
fi

# Docker
if [ "$platform" == "all" ] || [ "$platform" == "docker" ]; then
    ${base_dir}/platforms/docker/build.sh ${architecture} ${build_type}
fi

# Snap
if [ "$platform" == "snap" ]; then
    ${base_dir}/platforms/snap/build.sh ${architecture} ${build_type}
fi

# Windows
if [ "$platform" == "all" ] || [ "$platform" == "windows" ]; then
    docker_builder /app/platforms/windows/build.sh ${build_type}
fi

# macOS
if [ "$platform" == "all" ] || [ "$platform" == "macos" ]; then
    docker_builder /app/platforms/macos/build.sh ${architecture} ${build_type}
fi

# ESP32
if [ "$platform" == "all" ] || [ "$platform" == "esp32" ]; then
    if [ "$architecture" == "amd64" ]; then
        esp_architecture=esp32
    else
        esp_architecture="${architecture}"
    fi
    docker_builder /app/platforms/esp32/build.sh ${esp_architecture}
fi

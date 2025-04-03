#!/bin/bash
# Copyright (c) 2025 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
set -euo pipefail

husarnet_dir="/var/lib/husarnet"
files_dir=${husarnet_dir}/files

# Group superuser access mechanism
if ! grep -q "^husarnet:" /etc/group; then
    groupadd husarnet
fi

echo "***************************************************************"
echo " To use Husarnet without sudo add your user to husarnet group: "
echo "             sudo usermod -aG husarnet <username>              "
echo "                             OR                                "
echo "             sudo usermod -aG husarnet \$USER                  "
echo "***************************************************************"

# During install we want to make sure that all necessary files are present
# This is to ensure Husarnet can also run on systems with (mostly) read-only filesystems
if [ ! -d ${husarnet_dir} ]; then
  mkdir -p ${husarnet_dir}
fi

if [ ! -f ${husarnet_dir}/id ]; then
  husarnet-daemon --genid > ${husarnet_dir}/id # Identity has to exist
fi

if [ ! -f ${husarnet_dir}/daemon_api_token ]; then
  head /dev/urandom | tr -dc 'a-zA-Z0-9' | head -c 32 > ${husarnet_dir}/daemon_api_token
fi

# Daemon will refuse any config changes unless it's able to write to this file
if [ ! -f ${husarnet_dir}/config.json ]; then
  echo "{}" > ${husarnet_dir}/config.json # Empty dictionary is a valid config
fi

# This cache is a best-effort one - mostly to allow startup without Internet access
# It's not a problem if it can't be written unless this trait is important to you
if [ ! -f ${husarnet_dir}/cache.json ]; then
  echo "{}" > ${husarnet_dir}/cache.json # Empty dictionary is a valid cache
fi

# Make sure all the files in this directory support the husarnet group (non-recursive as we won't be managing i.e. hooks)
chgrp husarnet ${husarnet_dir}
find ${husarnet_dir} -maxdepth 1 -type f -exec chmod 660 {} +

# Husarnet files (hooks, etc.)
mkdir -p ${files_dir}
chmod 770 ${files_dir}
chgrp husarnet ${files_dir}

# Ignore installation of systemd service if systemd is not present
command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

husarnet daemon service-install --silent || exit 0

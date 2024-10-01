#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
set -euo pipefail
if ! grep -q "^husarnet:" /etc/group; then
    groupadd husarnet
fi
if [ ! -d "/var/lib/husarnet" ]; then
  mkdir -p /var/lib/husarnet
fi
chgrp husarnet /var/lib/husarnet -R
find /var/lib/husarnet -type d -exec chmod 770 {} +
find /var/lib/husarnet -type f -exec chmod 660 {} +
echo '**************************************************************'
echo "To use Husarnet without sudo add your user to husarnet group:"
echo "             sudo usermod -aG husarnet <username>            "
echo "                             OR                              "
echo "              sudo usermod -aG husarnet '$USER'              "
echo '**************************************************************'
command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

/usr/bin/husarnet daemon service-install --silent || exit 0

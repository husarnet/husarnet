#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
groupadd husarnet >> /dev/null 2>&1
chgrp husarnet /var/lib/husarnet -R
chmod g+rwx /var/lib/husarnet -R
sudo usermod -aG husarnet $SUDO_USER >> /dev/null 2>&1
command -v pidof >/dev/null || exit 0
command -v systemctl >/dev/null || exit 0

# Check whether system is *running* systemd
pidof -q systemd >/dev/null || exit 0

/usr/bin/husarnet daemon service-install --silent || exit 0

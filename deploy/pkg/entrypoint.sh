#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
set -euxo pipefail

# Check if the user provided an argument
if [ $# -ne 3 ]; then
    echo "Usage: $0 <key_id> <package_arch> <package_filename>"
    exit 1
fi

key_id="$1"
package_arch="$2"
package_filename="$3"

gpg --use-agent --no-autostart --output /release/${package_arch}/${package_filename}.sig --detach-sig /release/${package_arch}/${package_filename}

repo-add -s -k ${key_id} /release/${package_arch}/husarnet.db.tar.gz /release/${package_arch}/${package_filename}

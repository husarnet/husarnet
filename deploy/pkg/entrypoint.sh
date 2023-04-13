#!/bin/bash
set -euxo pipefail

# Check if the user provided an argument
if [ $# -neq 3 ]; then
    echo "Usage: $0 <key_id> <package_arch> <package_filename>"
    exit 1
fi

key_id="$1"
package_arch="$2"
package_filename="$3"

gpg --use-agent --output /release/${package_arch}/${package_filename}.sig --detach-sig /release/${package_arch}/${package_filename}

repo-add -s -k ${key_id} /release/${package_arch}/husarnet.db.tar.gz /release/${package_arch}/${package_filename}

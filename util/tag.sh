#!/bin/bash
# Copyright (c) 2022 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/bash-base.sh

git_version=$(git log -1 --pretty=%B | grep -v ^$ | cut -d ' ' -f 3)
repo_version=$(cat version.txt)

if [ ${git_version} != ${repo_version} ]; then
    echo "Last commit message does not contain the same version as the one found in files."
    exit 1
fi


# This one will trigger the release process
git tag v${repo_version}

# This one will make CLI importable as a Go library
git tag cli/v${repo_version}

git push origin --tags

echo "Release has started. Remember to fill in and publish the draft on Github!"

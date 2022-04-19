#!/usr/bin/env python3
import sys
import os.path
import os

ignore_dirs = {"build", ".git", ".vscode"}

old_copyright = """// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)"""

new_copyright = """// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt"""


def match_extension(path, allowed):
    _, ext = os.path.splitext(path)
    if ext.startswith("."):
        ext = ext[1:]

    return ext in allowed


def read_top_comment(file_path, prefix):
    with open(file_path, "r") as f:
        comment = []
        for line in f:
            if not line.startswith(prefix):
                break
            comment.append(line.strip())

        return "\n".join(comment)


def analyze_cpp(file_path):
    if not match_extension(file_path, {"h", "c", "cpp"}):
        return

    top_comment = read_top_comment(file_path, "//")
    if len(top_comment) < 10:
        print(f"{file_path} has no copyright at all")
        return

    if top_comment == old_copyright:
        print(f"{file_path} has an old copyright")
        return

    if top_comment == new_copyright:
        return

    print(f"{file_path} does not match any expected rules")
    print(top_comment)


def __main__():
    if not os.path.isdir("./.git"):
        print("Please start this program in a project's root directory")
        sys.exit(1)

    for (root, directories, files) in os.walk(".", topdown=True, followlinks=False):
        root_abs = os.path.abspath(root)
        directories[:] = [d for d in directories if d not in ignore_dirs]

        for file in files:
            full_path = os.path.join(root_abs, file)
            analyze_cpp(full_path)


if __name__ == "__main__":
    __main__()
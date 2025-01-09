#!/usr/bin/env python3
import sys
import os.path
import os

ignore_dirs = {"build", ".git", ".vscode", ".pio", ".pio_core"}

old_copyright = """// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)"""

new_copyright = """// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt"""

genqclient_exception = (
    """// Code generated by github.com/Khan/genqlient, DO NOT EDIT."""
)
gotools_exception = """//go:build tools
// +build tools"""


public_domain_exception = """// Public domain"""

issues_found = False


def generate_copyright_shell(copyright):
    return "\n".join(
        map(lambda line: "#" + line.removeprefix("//"), copyright.split("\n"))
    )


old_copyright_shell = generate_copyright_shell(old_copyright)
new_copyright_shell = generate_copyright_shell(new_copyright)


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


def analyze_source_cpp_golang(file_path):
    global issues_found

    top_comment = read_top_comment(file_path, "//")
    if top_comment == new_copyright:
        return

    if top_comment == genqclient_exception:
        return

    if top_comment == gotools_exception:
        return

    if top_comment == public_domain_exception:
        return

    if len(top_comment) < 10:
        print(f"{file_path} has no copyright at all")
        issues_found = True
        return

    if top_comment == old_copyright:
        print(f"{file_path} has an old copyright")
        issues_found = True
        return

    print(f"{file_path} does not match any expected rules")
    print(top_comment)
    issues_found = True


def analyze_source_shell(file_path):
    global issues_found

    top_comment = read_top_comment(file_path, "#")

    shebang, *comment = top_comment.split("\n")
    comment = "\n".join(comment)

    if not shebang.startswith("#!"):
        print(f"{file_path} has no shebang")
        issues_found = True
        return

    if comment == new_copyright_shell:
        return

    if len(comment) < 10:
        print(f"{file_path} has no copyright at all")
        issues_found = True
        return

    if comment == old_copyright_shell:
        print(f"{file_path} has an old copyright")
        issues_found = True
        return

    print(f"{file_path} does not match any expected rules")
    issues_found = True


def analyze_source(file_path):
    if match_extension(file_path, {"h", "c", "cpp", "go"}):
        analyze_source_cpp_golang(file_path)

    elif match_extension(file_path, {"sh"}):
        analyze_source_shell(file_path)


def __main__():
    print("[HUSARNET BS] Running copyright checker")

    if not os.path.isdir("./.git"):
        print("Please start this program in a project's root directory")
        sys.exit(1)

    for root, directories, files in os.walk(".", topdown=True, followlinks=False):
        root_abs = os.path.abspath(root)

        if "esp32/managed_components" in root_abs:
            continue  # Ignore some ESP32 files

        directories[:] = [d for d in directories if d not in ignore_dirs]

        for file in files:
            full_path = os.path.join(root_abs, file)
            analyze_source(full_path)

    if issues_found:
        sys.exit(1)


if __name__ == "__main__":
    __main__()

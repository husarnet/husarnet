#!/usr/bin/env python3
import subprocess
import sys
import os.path

husarnet_config_path = os.path.realpath(
    os.path.join(
        os.path.realpath(__file__), "..", "..", "husarnet", "husarnet_config.h"
    )
)
windows_installer_script_path = os.path.realpath(
    os.path.join(
        os.path.realpath(__file__), "..", "..", "windows", "installer", "script.iss"
    )
)


def get_new_version(line, today=None):
    if not today:
        today = subprocess.check_output(["date", "+%Y.%m.%d"]).strip().decode()

    parts = line.split('"')[1].split(".")
    existing_date = ".".join(parts[0:3])
    existing_version = int(parts[3])

    if existing_date == today:
        new_version = existing_version + 1
    else:
        new_version = 1

    return today + "." + str(new_version)


def bump_version(new_ver, line, today=None):
    return '#define HUSARNET_VERSION "' + new_ver + '"'


def test():
    def date_bump():
        today = "2021.04.10"
        line = '#define HUSARNET_VERSION "2021.04.09.55"'
        new_ver = get_new_version(line, today)

        assert (
            bump_version(new_ver, line, today)
            == '#define HUSARNET_VERSION "2021.04.10.1"'
        )

    def rev_bump():
        today = "2021.04.09"

        tests = [
            (
                '#define HUSARNET_VERSION "2021.04.09.1"',
                '#define HUSARNET_VERSION "2021.04.09.2"',
            ),
            (
                '#define HUSARNET_VERSION "2021.04.09.9"',
                '#define HUSARNET_VERSION "2021.04.09.10"',
            ),
            (
                '#define HUSARNET_VERSION "2021.04.09.10"',
                '#define HUSARNET_VERSION "2021.04.09.11"',
            ),
        ]

        for test in tests:
            new_ver = get_new_version(test[0], today)
            assert bump_version(new_ver, test[0], today) == test[1]

    date_bump()
    rev_bump()


version_to_insert = None


def replace_in_file(filepath, eol_char):
    global version_to_insert
    config = []
    with open(filepath, "r") as f:
        for line in f:
            if line.startswith("#define HUSARNET_VERSION "):
                if not version_to_insert:
                    version_to_insert = get_new_version(line)
                config.append(bump_version(version_to_insert, line))
            else:
                config.append(line.rstrip())

    with open(filepath, "w") as f:
        f.write(eol_char.join(config) + eol_char)


def main():
    replace_in_file(husarnet_config_path, "\n")
    replace_in_file(windows_installer_script_path, "\r\n")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        test()
    else:
        main()

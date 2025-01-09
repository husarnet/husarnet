#!/usr/bin/env python3
import os.path

version_file_path = os.path.realpath(
    os.path.join(os.path.realpath(__file__), "..", "..", "version.txt")
)

husarnet_config_path = os.path.realpath(
    os.path.join(os.path.realpath(__file__), "..", "..", "core", "husarnet", "husarnet_config.h")
)
windows_installer_script_path = os.path.realpath(
    os.path.join(
        os.path.realpath(__file__), "..", "..", "platforms", "windows", "script.iss"
    )
)

cli_version_path = os.path.realpath(
    os.path.join(os.path.realpath(__file__), "..", "..", "cli", "constants", "version.go")
)

snap_yaml_path = os.path.realpath(
    os.path.join(
        os.path.realpath(__file__), "..", "..", "platforms", "snap", "snapcraft.yaml"
    )
)


def get_new_version(old):
    parts = old.split(".")
    last_part = parts[-1]

    if last_part == "#":
        last_part = 0
    else:
        last_part = int(last_part) + 1

    parts[-1] = str(last_part)

    return ".".join(parts)


def get_current_version_from_file():
    f = open(version_file_path, "r")
    ver = f.read().rstrip()
    f.close()
    return ver


def update_version_file(new_ver):
    f = open(version_file_path, "w")
    f.write(new_ver)
    f.close()


def get_new_version_string_for_cpp_and_iss(new_ver):
    return '#define HUSARNET_VERSION "' + new_ver + '"'


def get_new_version_string_for_go(new_ver):
    return 'const Version string = "' + new_ver + '"'


def get_new_version_string_for_yaml(new_ver):
    return 'version: "' + new_ver + '"'


def replace_in_file(new_ver_string, searched_string, filepath, eol_char):
    config = []
    with open(filepath, "r") as f:
        for line in f:
            if line.startswith(searched_string):
                config.append(new_ver_string)
            else:
                config.append(line.rstrip())

    with open(filepath, "w") as f:
        f.write(eol_char.join(config) + eol_char)


def main():
    new_ver = get_new_version(get_current_version_from_file())
    replace_in_file(
        get_new_version_string_for_cpp_and_iss(new_ver),
        "#define HUSARNET_VERSION ",
        husarnet_config_path,
        "\n",
    )
    replace_in_file(
        get_new_version_string_for_cpp_and_iss(new_ver),
        "#define HUSARNET_VERSION ",
        windows_installer_script_path,
        "\r\n",
    )
    replace_in_file(
        get_new_version_string_for_go(new_ver),
        "const Version string =",
        cli_version_path,
        "\n",
    )
    replace_in_file(
        get_new_version_string_for_yaml(new_ver),
        "version: ",
        snap_yaml_path,
        "\n",
    )

    update_version_file(new_ver)


if __name__ == "__main__":
    main()

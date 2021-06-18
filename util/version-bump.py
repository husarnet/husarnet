#!/usr/bin/env python3
import subprocess
import sys
import os.path


husarnet_config_path = os.path.realpath(os.path.join(os.path.realpath(__file__), '..', '..', 'core', 'husarnet_config.h'))


def bump_version(line, today=None):
    if not today:
        today = subprocess.check_output(["date", '+%Y.%m.%d']).strip().decode()

    parts = line.split('"')[1].split('.')
    existing_date = '.'.join(parts[0:3])
    existing_version = int(parts[3])

    if existing_date == today:
        new_version = existing_version + 1
    else:
        new_version = 1

    return '#define HUSARNET_VERSION "' + today + '.' + str(new_version) + '"'


def test():
    def date_bump():
        today = '2021.04.10'

        assert bump_version('#define HUSARNET_VERSION "2021.04.09.55"', today=today) == '#define HUSARNET_VERSION "2021.04.10.1"'

    def rev_bump():
        today = '2021.04.09'

        tests = [
            ('#define HUSARNET_VERSION "2021.04.09.1"', '#define HUSARNET_VERSION "2021.04.09.2"'),
            ('#define HUSARNET_VERSION "2021.04.09.9"', '#define HUSARNET_VERSION "2021.04.09.10"'),
            ('#define HUSARNET_VERSION "2021.04.09.10"', '#define HUSARNET_VERSION "2021.04.09.11"'),
        ]

        for test in tests:
            assert bump_version(test[0], today=today) == test[1]

    date_bump()
    rev_bump()


def main():
    config = []
    with open(husarnet_config_path, 'r') as f:
        for line in f:
            if line.startswith('#define HUSARNET_VERSION '):
                config.append(bump_version(line))
            else:
                config.append(line.rstrip())

    with open(husarnet_config_path, 'w') as f:
        f.write('\n'.join(config) + '\n')


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        test()
    else:
        main()

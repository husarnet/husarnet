#!/bin/bash

# TODO: comments & error handling

set -e

platforms/esp32/idf.py -p "$1" -b 921600 flash
tio -b 115200 "$1"

#!/bin/bash

esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 ../../build/esp32/bootloader/bootloader.bin 0x8000 ../../build/esp32/partition_table/partition-table.bin 0x10000 ../../build/esp32/husarnet_esp32_core.bin
# Building for Arduino

```
# setup compiler toolchain according to ESP32 instructions
git clone --recursive https://github.com/husarnet/esp-idf
export IDF_PATH=$PWD/esp-idf
git clone git@tools.husarion.com:husarion/husarnet.git
git clone git@tools.husarion.com:husarion/arduino-esp32.git
git clone git@tools.husarion.com:husarion/esp32-arduino-lib-builder.git
(cd esp32-arduino-lib-builder && ./build.sh)
(cd arduino-esp32 && ./release.sh)
```

For testing, symlink arduino-esp32 repo to $HOME/Arduino/hardware/espressif/esp32 - Arduino IDE will pick it up automatically after restart.

Copy arduino-husarnet-esp32-v1.zip to www:/var/www/files/arduino. Modify package_esp32_husarnet_index.json with new filename and SHA256 hash.

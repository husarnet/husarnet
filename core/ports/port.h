// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"

#ifdef ESP_PLATFORM
#define PORT_ESP32
#define PORT_NAME "ESP32"
#include "husarnet/ports/esp32/port.h"
#endif  // ESP_PLATFORM

#ifdef _WIN32
#define PORT_WINDOWS
#define PORT_NAME "Windows"
#include "husarnet/ports/windows/port.h"
#endif  // _WIN32

#ifdef __linux__
#define PORT_LINUX
#define PORT_NAME "Linux"
#include "husarnet/ports/linux/port.h"
#endif

#ifdef __APPLE__
#define PORT_MACOS
#define PORT_NAME "MacOS"
#include "husarnet/ports/macos/port.h"
#endif

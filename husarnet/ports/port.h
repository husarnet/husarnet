// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/privileged_interface.h"

#ifdef ESP_PLATFORM
#include "husarnet/ports/esp32/port.h"
#endif  // ESP_PLATFORM

#ifdef _WIN32
#include "husarnet/ports/windows/port.h"
#endif  // _WIN32

#ifdef __linux__
#include "husarnet/ports/unix/port.h"
#endif

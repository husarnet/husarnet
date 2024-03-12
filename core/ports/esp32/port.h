// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <lwip/inet.h>
#include "tun.h"
#include "sdkconfig.h"

#define SOCKFUNC(name) ::name
#define SOCKFUNC_close SOCKFUNC(close)

// Get task priorities from the Kconfig file
#define NGSOCKET_TASK_PRIORITY            CONFIG_HUSARNET_NGSOCKET_TASK_PRIORITY
#define HUSARNET_TASK_PRIORITY            CONFIG_HUSARNET_HUSARNET_TASK_PRIORITY
#define WEBSETUP_PERIODIC_TASK_PRIORITY   CONFIG_HUSARNET_WEBSETUP_PERIODIC_TASK_PRIORITY
#define WEBSETUP_CONNECTION_TASK_PRIORITY CONFIG_HUSARNET_WEBSETUP_CONNECTION_TASK_PRIORITY

namespace Port {
  extern SemaphoreHandle_t notifyReadySemaphore;
}
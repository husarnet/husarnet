// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <lwip/inet.h>

#include "sdkconfig.h"
#include "tun.h"

#define SOCKFUNC(name) ::name
#define SOCKFUNC_close SOCKFUNC(close)

// Get task priorities from the Kconfig file
#define CONFIG_MANAGER_TASK_PRIORITY CONFIG_HUSARNET_CONFIG_MANAGER_TASK_PRIORITY
#define EVENTBUS_TASK_PRIORITY CONFIG_HUSARNET_EVENTBUS_TASK_PRIORITY
#define HEARTBEAT_TASK_PRIORITY CONFIG_HUSARNET_HEARTBEAT_TASK_PRIORITY
#define NGSOCKET_TASK_PRIORITY CONFIG_HUSARNET_NGSOCKET_TASK_PRIORITY

namespace Port {
  extern SemaphoreHandle_t notifyReadySemaphore;
}

// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

// Task priorities are only used on embedded platforms,
// where the RTOS is used. This file is included by Linux,
// Windows and MacOS ports to provide placeholder values as
// we don't set invidual thread priorities on these platforms.

#define CONFIG_MANAGER_TASK_PRIORITY 0
#define EVENTBUS_TASK_PRIORITY 0
#define HEARTBEAT_TASK_PRIORITY 0
#define NGSOCKET_TASK_PRIORITY 0
#define WINTUN_READER_TASK_PRIORITY 0

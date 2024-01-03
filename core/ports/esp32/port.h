// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

// #define LWIP_COMPAT_SOCKETS 0
// #define NGSOCKET_USE_LWIP
// #include <lwip/netdb.h>
// #include <lwip/sockets.h>

#include <lwip/inet.h>
#include "user_interface.h"
// #define SOCKFUNC(name) ::lwip_##name##_r

#define SOCKFUNC(name) ::name
#define SOCKFUNC_close SOCKFUNC(close)

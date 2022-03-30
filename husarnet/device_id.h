// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "fstring.h"

using DeviceId = fstring<16>;

static const DeviceId BadDeviceId =
    DeviceId("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");  //__attribute__((unused));

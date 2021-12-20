#pragma once
#include "fstring.h"
using DeviceId = fstring<16>;

static const DeviceId BadDeviceId =
    DeviceId("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");  //__attribute__((unused));

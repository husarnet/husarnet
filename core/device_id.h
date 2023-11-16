// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"

using DeviceId = fstring<16>;

static const DeviceId BadDeviceId =
    DeviceId("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");  //__attribute__((unused));

inline IpAddress deviceIdToIpAddress(DeviceId id)
{
  return IpAddress::fromBinary(id);
}

inline DeviceId deviceIdFromIpAddress(IpAddress ip)
{
  return ip.toBinary();
}

inline const char* deviceIdToCStr(DeviceId id)
{
  return reinterpret_cast<char*>(id.data());
}

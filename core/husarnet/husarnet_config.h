// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ports/port.h"

#include "husarnet/ipaddress.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#define HUSARNET_VERSION "2.0.305"
#define HUSARNET_USER_AGENT "Husarnet," PORT_NAME "," PORT_ARCH "," HUSARNET_VERSION

#define BASESERVER_PORT 443
#define MULTICAST_PORT 5581

#ifdef ESP_PLATFORM
#define JSON_INDENT_SPACES 0
#else
#define JSON_INDENT_SPACES 4
#endif

static const int MAX_QUEUED_PACKETS = 10;

// this is IPv4 AD-HOC multicast address (224.3.252.148)
const IpAddress MULTICAST_ADDR_4 = IpAddress::parse("::ffff:E003:FC94");
const IpAddress MULTICAST_ADDR_6 = IpAddress::parse("ff02:88bb:31e4:95f7:2b87:6b52:e112:19ac");

const IpAddress multicastDestination = IpAddress::parse("ff15:f2d3:a389::");

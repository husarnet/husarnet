// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <map>
#include <string>

#include "husarnet/ipaddress.h"

struct IpAddress;

bool updateHostsFileInternal(std::map<std::string, IpAddress> data);
bool validateHostname(std::string hostname);

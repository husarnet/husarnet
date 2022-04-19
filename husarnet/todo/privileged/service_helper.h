// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include <vector>
#include "husarnet/ipaddress.h"

namespace ServiceHelper {
bool validateHostname(std::string hostname);
bool modifyHostname(std::string hostname);
std::vector<std::pair<IpAddress, std::string>> getHostsFileAtStartup();
std::vector<std::pair<IpAddress, std::string>> getCurrentHostsFileInternal();
void startServiceHelperProc(std::string configDir);
bool updateHostsFile(std::vector<std::pair<IpAddress, std::string>> data);
bool updateHostsFileInternal(
    std::vector<std::pair<IpAddress, std::string>> data);
}  // namespace ServiceHelper

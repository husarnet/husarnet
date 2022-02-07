#pragma once
#include <string>
#include <vector>
#include "ipaddress.h"

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

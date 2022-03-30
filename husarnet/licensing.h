// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <list>
#include <string>
#include "husarnet/ipaddress.h"
#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

json retrieveLicenseJSON(std::string dashboardHostname);

class License {
  std::string dashboardUrl;
  IpAddress websetupAddress;
  std::vector<IpAddress> baseServerAddresses;

 public:
  License(std::string dashboardHostname);
  std::string getDashboardUrl();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();
};

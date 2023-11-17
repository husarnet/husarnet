// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <list>
#include <string>

#include "husarnet/ipaddress.h"

#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

json retrieveLicenseJson(std::string dashboardHostname, bool abortOnFailure);

class License {
  std::string dashboardFqdn;
  IpAddress websetupAddress;
  std::vector<IpAddress> baseServerAddresses;

 public:
  License(std::string dashboardHostname);
  std::string getDashboardFqdn();
  IpAddress getWebsetupAddress();
  std::vector<IpAddress> getBaseServerAddresses();
  static bool validateDashboard(std::string dashboardHostname);
};

#pragma once

#include <list>
#include <string>
#include "ipaddress.h"
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

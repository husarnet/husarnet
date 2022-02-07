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
  std::list<IpAddress> baseServerAddresses;

 public:
  License(std::string dashboardHostname);
  std::string getDashboardUrl();
  IpAddress getWebsetupAddress();
  std::list<IpAddress> getBaseServerAddresses();
};

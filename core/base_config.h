#pragma once

#include <string>
#include <vector>

#include "ipaddress.h"

class BaseConfig {
  std::vector<InetAddress> baseTcpAddresses;
  std::string dashboardUrl;
  std::string baseDnsAddress;
  std::vector<std::string> defaultWebsetupHosts;
  std::string defaultJoinHost;

 public:
  explicit BaseConfig(const std::string& licenseFile);

  bool isDefault() const;
  const std::vector<InetAddress>& getBaseTcpAddresses() const;
  const std::string& getDashboardUrl() const;
  const std::string& getBaseDnsAddress() const;
  const std::vector<std::string>& getDefaultWebsetupHosts() const;
  const std::string& getDefaultJoinHost() const;

  static BaseConfig* create(const std::string);
};

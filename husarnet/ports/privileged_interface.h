// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <list>
#include <map>
#include <string>
#include <vector>
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"

namespace Privileged {
  void init();   // Should be called as early as possible
  void start();  // Should be called as soon as the parent process is ready to
                 // drop all privileges

  std::string readConfig();
  void writeConfig(std::string);

  Identity readIdentity();
  void writeIdentity(Identity identity);

  std::string readApiSecret();
  void rotateApiSecret();

  std::vector<IpAddress> getLocalAddresses();

  // std::string updateLicenseFile();

  std::string getSelfHostname();
  bool setSelfHostname(std::string newHostname);

  void updateHostsFile(std::map<std::string, IpAddress> data);

  // void saveLicenseFile(std::string contents);
  // std::string readLicenseFile();
}  // namespace Privileged

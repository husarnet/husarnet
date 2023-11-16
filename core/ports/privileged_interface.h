// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <ctime>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"

struct IpAddress;

namespace Privileged {
  void init();   // Should be called as early as possible
  void start();  // Should be called as soon as the parent process is ready to
                 // drop all privileges

  void createConfigDirectories();

  void dropCapabilities();

  std::string getConfigPath();
  std::string getIdentityPath();
  std::string getApiSecretPath();
  std::string getLegacyConfigPath();
  std::string getLicenseJsonPath();
  std::string getNotificationFilePath();

  std::string readLicenseJson();
  void writeLicenseJson(std::string);

  std::string readNotificationFile();
  void writeNotificationFile(std::string);

  std::string readConfig();
  void writeConfig(std::string);

  Identity readIdentity();
  Identity createIdentity();
  bool checkValidIdentityExists();
  void writeIdentity(Identity identity);

  IpAddress resolveToIp(const std::string& hostname);

  std::string readApiSecret();
  // TODO would be nice to move this somewhere else - like HusarnetManager
  void rotateApiSecret();

  std::vector<std::pair<std::time_t, std::string>> readNotifications();
  void writeNotifications(
      std::vector<std::pair<std::time_t, std::string>> list);

  std::vector<IpAddress> getLocalAddresses();

  std::string getSelfHostname();
  bool setSelfHostname(const std::string& newHostname);

  void updateHostsFile(const std::map<std::string, IpAddress>& data);

  void notifyReady();

  void runScripts(const std::string& path);
  bool checkScriptsExist(const std::string& path);
}  // namespace Privileged

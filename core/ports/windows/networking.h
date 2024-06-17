// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

class HusarnetManager;

class WindowsNetworking {
 private:
  std::string netshName;

  std::string getNetshNameForGuid(std::string guid) const;
  int callWindowsCmd(std::string cmd) const;
  void insertFirewallRule(const std::string firewallRuleName) const;
  void deleteFirewallRules(const std::string firewallRuleName) const;

 public:
  WindowsNetworking() : netshName("Husarnet")
  {
  }
  void setupNetworkInterface(
      HusarnetManager* manager,
      const std::string interfaceName);
  void allowHusarnetThroughWindowsFirewall(const std::string firewallRuleName);
};

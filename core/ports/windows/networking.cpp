// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/windows/networking.h"

#include <string>

#include <stdlib.h>

#include "husarnet/ports/port.h"
#include "husarnet/ports/shared_unix_windows/filesystem.h"

#include "husarnet/identity.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

std::string WindowsNetworking::getNetshNameForGuid(std::string guid) const
{
  std::string path =
      "SYSTEM\\CurrentControlSet\\Control\\Network\\{4d36e972-e325-11ce-bfc1-"
      "08002be10318}\\" +
      guid + "\\connection";

  DWORD dwType = REG_SZ;
  HKEY hKey = 0;
  char value[1024];
  DWORD value_length = 1024;
  RegOpenKey(HKEY_LOCAL_MACHINE, path.c_str(), &hKey);
  RegQueryValueEx(hKey, "name", NULL, &dwType, (LPBYTE)&value, &value_length);

  return std::string(value);
}

int WindowsNetworking::callWindowsCmd(std::string cmd) const
{
  return system(("\"" + cmd + "\"").c_str());
}
void WindowsNetworking::setupNetworkInterface(std::string interfaceName)
{
  auto identityPath = std::string(getenv("PROGRAMDATA")) + "\\husarnet\\id";
  auto identity = Identity::deserialize(Port::readFile(identityPath));

  std::string sourceNetshName = getNetshNameForGuid(interfaceName);
  LOG_WARNING("sourceNetshName: %s", sourceNetshName.c_str());
  if(netshName != sourceNetshName) {
    if(callWindowsCmd(
           "netsh interface set interface name = \"" + sourceNetshName +
           "\" newname = \"" + netshName + "\"") == 0) {
      LOG_INFO("renamed successfully");
    } else {
      netshName = sourceNetshName;
      LOG_WARNING("rename failed");
    }
  }
  std::string quotedName = "\"" + netshName + "\"";

  callWindowsCmd(
      "netsh interface ipv6 add neighbors " + quotedName +
      " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001 52-54-00-fc-94-4d");
  std::string myIp = IpAddress::fromBinary(identity.getDeviceId()).str();
  LOG_INFO("myIp is: %s", myIp.c_str());
  callWindowsCmd(
      "netsh interface ipv6 add address " + quotedName + " " + myIp + "/128");
  callWindowsCmd(
      "netsh interface ipv6 add route "
      "fc94:8385:160b:88d1:c2ec:af1b:06ac:0001/128 " +
      quotedName);
  callWindowsCmd(
      "netsh interface ipv6 add route fc94::/16 " + quotedName +
      " fc94:8385:160b:88d1:c2ec:af1b:06ac:0001");
}

void WindowsNetworking::insertFirewallRule(std::string firewallRuleName) const
{
  LOG_INFO("Installing rule: %s in Windows Firewall", firewallRuleName.c_str());

  std::string insertCmd =
      "powershell New-NetFirewallRule -DisplayName " + firewallRuleName +
      " -Direction "
      "Inbound -Action Allow -LocalAddress fc94::/16 -RemoteAddress fc94::/16 "
      "-InterfaceAlias \"\"\"" +
      netshName + "\"\"\"";

  callWindowsCmd(insertCmd);
}

void WindowsNetworking::deleteFirewallRules(
    const std::string firewallRuleName) const
{
  // Remove-NetFirewallRule will remove ALL matching rules
  std::string deleteCmd =
      "powershell Remove-NetFirewallRule -DisplayName " + firewallRuleName;
  callWindowsCmd(deleteCmd);
}

void WindowsNetworking::allowHusarnetThroughWindowsFirewall(
    const std::string firewallRuleName)
{
  std::string checkCmd =
      "powershell -command \"Get-NetFirewallRule -DisplayName " +
      firewallRuleName + " 2>&1 >$null \"";
  int returnCode = callWindowsCmd(checkCmd);

  if(returnCode == 0) {
    // Due to bug in earlier versions of Windows client, the rule was inserted
    // on each start of the service, which means users could have a lot of
    // redundant rules in their firewall. we delete them here to cleanup
    LOG_WARNING(
        "Firewall rule: %s already exists! Cleaning up...",
        firewallRuleName.c_str());
    deleteFirewallRules(firewallRuleName);
  }
  insertFirewallRule(firewallRuleName);
}

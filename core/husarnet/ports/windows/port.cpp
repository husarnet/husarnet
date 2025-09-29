// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "husarnet/ports/port.h"

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "process.h"
#include "shlwapi.h"
#include "sysinfoapi.h"

thread_local const char* threadName = nullptr;

static void runThread(void* arg)
{
  auto f = (std::pair<char*, std::function<void()>>*)arg;
  threadName = f->first;
  f->second();
}

static std::list<std::string> getAvailableScripts(const std::string& path)
{
  char* conf_path = std::getenv("PROGRAMDATA");
  auto fullPath = std::string(conf_path) + "\\Husarnet\\" + path;
  std::filesystem::path dir(fullPath);
  std::list<std::string> results;

  LOG_INFO(("Checking for valid hooks under " + fullPath).c_str());

  if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
    LOG_INFO((fullPath + " does not exist").c_str());
    return results;
  }

  for(const auto& entry : std::filesystem::directory_iterator(dir)) {
    if(entry.path().extension() != ".ps1") {
      LOG_INFO((entry.path().string() + " has wrong extension").c_str());
      continue;
    }

    if((entry.status().permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::owner_exec) {
      LOG_INFO((entry.path().string() + " is not executable").c_str());
      continue;
    }

    results.push_back(entry.path().string());
  }

  return results;
}

namespace Port {
  void init()
  {
    WSADATA wsaData;
    WSAStartup(0x202, &wsaData);

    fatInit();
  }

  void threadStart(std::function<void()> func, const char* name, int stack, int priority)
  {
    auto* f = new std::pair<const char*, std::function<void()>>(name, std::move(func));
    _beginthread(runThread, 0, f);
  }

  void notifyReady()
  {
    // Not implemented in Windows port.
  }

  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName)
  {
    LOG_ERROR("getIpAddressFromInterfaceName is not implemented");
    return IpAddress();
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> result;

    PIP_ADAPTER_ADDRESSES buffer;
    unsigned long buffer_size = 20000;

    while(true) {
      int ret;

      buffer = (PIP_ADAPTER_ADDRESSES) new char[buffer_size];

      ret = GetAdaptersAddresses(
          AF_UNSPEC,
          GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_DNS_SERVER |
              GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST,
          0, buffer, &buffer_size);
      if(ret != ERROR_BUFFER_OVERFLOW)
        break;
      delete[] buffer;
      buffer = nullptr;
      buffer_size *= 2;
    }
    PIP_ADAPTER_ADDRESSES current_adapter = buffer;
    while(current_adapter) {
      PIP_ADAPTER_UNICAST_ADDRESS current = current_adapter->FirstUnicastAddress;
      while(current) {
        auto ss = reinterpret_cast<sockaddr_storage*>(current->Address.lpSockaddr);
        InetAddress addr = OsSocket::ipFromSockaddr(*ss);
        result.push_back(addr.ip);
        current = current->Next;
      }
      current_adapter = current_adapter->Next;
    }

    delete[] buffer;
    buffer = nullptr;
    return result;
  }

  UpperLayer* startTun(const HusarnetAddress& myAddress, const std::string& interfaceName)
  {
    auto tun = new Tun(myAddress);
    auto started = tun->start();

    if(!started) {
      LOG_CRITICAL("TUN bringup failed; Husarnet won't work");
    } else {
      LOG_INFO("TUN config OK");
    }

    // these can take quite a few seconds
    // but at the same time they're required for this stuff to even work,
    // so we do it before starting reading from TUN
    LOG_INFO("netsh adapter name is %s", tun->getAdapterName().c_str());
    setupRoutingTable(tun->getAdapterName());
    setupFirewall(tun->getAdapterName());

    tun->startReaderThread();
    LOG_INFO("started reading from TUN adapter")

    return tun;
  }

  std::string getSelfHostname()
  {
    TCHAR buf[256];
    DWORD size = _countof(buf);
    bool result = GetComputerNameEx(ComputerNamePhysicalDnsHostname, buf, &size);
    if(result) {
      return std::string(buf);
    }
    LOG_WARNING("Cant retrieve hostname");

    // TODO long-term: implement some sort of a fallback based on hardware
    // numbers

    return "windows-pc";
  }

  bool setSelfHostname(const std::string& newHostname)
  {
    // Not implemented in Windows port.
    // This can be done through SetComputerNameEx()
    // Caveat is that the reboot is required for the change to be picked up
    // My opinion is to not touch the netbios hostname, and just add entry into
    // hosts
    return true;
  }

  int callWindowsCmd(std::string cmd)
  {
    return std::system(("\"" + cmd + "\"").c_str());
  }

  bool runScripts(const std::string& eventName)
  {
    for(const auto& scriptPath : getAvailableScripts(eventName)) {
      LOG_INFO(("Running " + scriptPath).c_str());
      std::system(("powershell.exe -File " + scriptPath).c_str());
    }

    return true;
  }

  bool checkScriptsExist(const std::string& eventName)
  {
    return !getAvailableScripts(eventName).empty();
  }

  void setupRoutingTable(const std::string& netshName)
  {
    std::string quotedName = "\"" + netshName + "\"";
    callWindowsCmd("netsh interface ipv6 add route fc94::/16 " + quotedName);
  }

  void insertFirewallRule(const std::string& netshName, const std::string& firewallRuleName)
  {
    LOG_INFO("Installing rule: %s in Windows Firewall", firewallRuleName.c_str());

    std::string insertCmd = "powershell \"New-NetFirewallRule -DisplayName " + firewallRuleName +
                            " -Direction Inbound -Action Allow"
                            " -LocalAddress fc94::/16 -RemoteAddress fc94::/16 "
                            "-InterfaceAlias " +
                            netshName + " | Out-Null\"";

    callWindowsCmd(insertCmd);
  }

  void deleteFirewallRules(const std::string& firewallRuleName)
  {
    // Remove-NetFirewallRule will remove ALL matching rules
    std::string deleteCmd = "powershell \"Remove-NetFirewallRule -DisplayName " + firewallRuleName + " | Out-Null \"";
    callWindowsCmd(deleteCmd);
  }

  void setupFirewall(const std::string& netshName)
  {
    const std::string firewallRuleName{"AllowHusarnet"};
    // Due to bug in earlier versions of Windows client, the rule was inserted
    // on each start of the service, which means users could have a lot of
    // redundant rules in their firewall; that's why we start with the cleanup
    deleteFirewallRules(firewallRuleName);
    insertFirewallRule(netshName, firewallRuleName);
  }
}  // namespace Port

// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port.h"

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/fat/filesystem.h"
#include "husarnet/ports/macos/tun.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "enum.h"
#include "nlohmann/json.hpp"
#include "stdio.h"

static bool isInterfaceBlacklisted(std::string name)
{
  if(name.size() > 2 && name.substr(0, 2) == "zt")
    return true;  // sending traffic over ZT may cause routing loops

  if(name == "hnetl2")
    return true;

  return false;
}

static IpAddress ipAddressFromSockaddr(const struct sockaddr* addr)
{
  if(addr == nullptr)
    return IpAddress();

  if(addr->sa_family == AF_INET) {
    uint32_t addr32 = ((const struct sockaddr_in*)addr)->sin_addr.s_addr;
    return IpAddress::fromBinary4(addr32);
  }

  if(addr->sa_family == AF_INET6) {
    const struct in6_addr& a6 = ((const struct sockaddr_in6*)addr)->sin6_addr;
    return IpAddress::fromBinary((const char*)a6.s6_addr);
  }

  return IpAddress();
}

namespace Port {
  void init()
  {
    fatInit();
  }

  void notifyReady()
  {
    // NOOP
  }

  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName)
  {
    struct ifaddrs* ifAddrs = nullptr;
    if(getifaddrs(&ifAddrs) != 0) {
      HLOG_ERROR("getifaddrs failed // {error}", strerror(errno));
      return IpAddress();
    }

    // Prefer an IPv4 address (this is used to pick a bind address for the API
    // server), but fall back to a routable IPv6 one if that's all there is.
    IpAddress v4, v6;
    for(struct ifaddrs* ifa = ifAddrs; ifa != nullptr; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr == nullptr || ifa->ifa_name == nullptr)
        continue;
      if(interfaceName != ifa->ifa_name)
        continue;

      IpAddress address = ipAddressFromSockaddr(ifa->ifa_addr);
      if(address.isInvalid() || address.isLinkLocal() || address.isLoopback())
        continue;

      if(ifa->ifa_addr->sa_family == AF_INET && v4.isInvalid())
        v4 = address;
      else if(ifa->ifa_addr->sa_family == AF_INET6 && v6.isInvalid())
        v6 = address;
    }

    freeifaddrs(ifAddrs);

    IpAddress result = v4.isValid() ? v4 : v6;
    if(result.isInvalid())
      HLOG_WARNING("no usable address found on interface // {interface}", interfaceName);
    return result;
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;

    struct ifaddrs* ifAddrs = nullptr;
    if(getifaddrs(&ifAddrs) != 0) {
      HLOG_ERROR("getifaddrs failed // {error}", strerror(errno));
      return ret;
    }

    for(struct ifaddrs* ifa = ifAddrs; ifa != nullptr; ifa = ifa->ifa_next) {
      if(ifa->ifa_addr == nullptr || ifa->ifa_name == nullptr)
        continue;
      if(isInterfaceBlacklisted(ifa->ifa_name))
        continue;

      IpAddress address = ipAddressFromSockaddr(ifa->ifa_addr);

      if(address.isInvalid() || address.isLoopback() || address.isLinkLocal() || address.isFC94())
        continue;

      ret.push_back(address);
    }

    freeifaddrs(ifAddrs);

    return ret;
  }

  UpperLayer* startTun(const HusarnetAddress& myAddress, const std::string& interfaceName)
  {
    (void)interfaceName;  // ignore Linux-centric hnet0, setup utunX

    auto tunTap = new Tun();
    auto utunName = tunTap->getName();

    if(!tunTap->isRunning() || utunName.empty()) {
      HLOG_CRITICAL("could not create utun interface, networking will not work");
      return tunTap;
    }

    HLOG_INFO("starting utun // {interface}", utunName.c_str());

    if(system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
       system(("sysctl net.ipv6.conf." + utunName + ".disable_ipv6=0").c_str()) != 0) {
      HLOG_WARNING("failed to enable IPv6 (may be harmless)");
    }

    system(("ifconfig " + utunName + " inet6 " + myAddress.toString()).c_str());
    system(("route -nv add -inet6 fc94::/16 -interface " + utunName).c_str());
    // TODO multicast, right?

    return tunTap;
  }

  std::string getSelfHostname()
  {
    const std::string hostnamePath = "/tmp/hostname";
    const std::string command = "scutil --get LocalHostName > " + hostnamePath + " 2>/dev/null";
    std::system(command.c_str());
    if(!isFile(hostnamePath)) {
      HLOG_WARNING(
          "Unable to retrieve hostname from scutil, defaulting to "
          "macos-device");
      return "macos-device";
    }
    auto hostname = readFile(hostnamePath);
    if(!hostname.empty()) {
      return hostname;
    }
    return "macos-device";
  }

  bool setSelfHostname(const std::string& newHostname)
  {
    // Not implemented in MacOS port (on purpose).
    return true;
  }

  // TODO implement hooks
  bool runScripts(const std::string& path)
  {
    return false;
  }

  // TODO implement hooks
  bool checkScriptsExist(const std::string& path)
  {
    return false;
  }

}  // namespace Port

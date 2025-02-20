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

#include "husarnet/config_storage.h"
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

static void getLocalIpv4Addresses(std::vector<IpAddress>& ret)
{
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  struct ifconf configuration {};

  if(fd < 0)
    return;

  if(ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;
  configuration.ifc_buf = (char*)malloc(configuration.ifc_len);
  if(ioctl(fd, SIOCGIFCONF, &configuration) < 0)
    goto error;

  for(int i = 0; i < (int)(configuration.ifc_len / sizeof(ifreq)); i++) {
    struct ifreq& request = configuration.ifc_req[i];
    std::string ifname = request.ifr_name;
    if(isInterfaceBlacklisted(ifname))
      continue;
    struct sockaddr* addr = &request.ifr_ifru.ifru_addr;
    if(addr->sa_family != AF_INET)
      continue;

    uint32_t addr32 = ((sockaddr_in*)addr)->sin_addr.s_addr;
    IpAddress address = IpAddress::fromBinary4(addr32);

    if(addr32 == 0x7f000001u)
      continue;

    ret.push_back(address);
  }
  free(configuration.ifc_buf);
error:
  close(fd);
  return;
}

static void getLocalIpv6Addresses(std::vector<IpAddress>& ret)
{
  // on MacOS there is no /proc, so we can't use /proc/self/net/if_inet6
  // like in linux port; need to figure out different way
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
    LOG_ERROR("getIpAddressFromInterfaceName is not implemented");
    return IpAddress();
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;

    getLocalIpv4Addresses(ret);
    getLocalIpv6Addresses(ret);

    return ret;
  }

  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    std::string myIp = manager->getIdentity()->getDeviceId().str();

    auto tunTap = new TunTap();
    auto interfaceName = tunTap->getName();
    LOG_INFO("our utun interface name is %s", interfaceName.c_str());

    if(system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
       system(("sysctl net.ipv6.conf." + interfaceName + ".disable_ipv6=0")
                  .c_str()) != 0) {
      LOG_WARNING("failed to enable IPv6 (may be harmless)");
    }

    system(("ifconfig " + interfaceName + " inet6 " + myIp).c_str());
    system(
        ("route -nv add -inet6 fc94::/16 -interface " + interfaceName).c_str());
    // TODO multicast, right?

    return tunTap;
  }

  std::string getSelfHostname()
  {
    const std::string hostnamePath = "/tmp/hostname";
    const std::string command =
        "scutil --get LocalHostName > " + hostnamePath + " 2>/dev/null";
    std::system(command.c_str());
    if(!isFile(hostnamePath)) {
      LOG_WARNING(
          "Unable to retrieve hostname from scutil, defaulting to "
          "macos-device");
      return "macos-device";
    }
    return rtrim(readFile(hostnamePath).value_or("macos"));
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

// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port.h"

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <ares.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/linux/tun.h"
#include "husarnet/ports/port.h"

#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "enum.h"

static FILE* if_inet6;

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
  fseek(if_inet6, 0, SEEK_SET);
  while(true) {
    char buf[100];
    if(fgets(buf, sizeof(buf), if_inet6) == nullptr)
      break;
    std::string line = buf;
    if(line.size() < 32)
      continue;
    auto ip = IpAddress::fromBinary(decodeHex(line.substr(0, 32)));
    if(ip.isLinkLocal())
      continue;
    if(ip == IpAddress::fromBinary("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"))
      continue;

    std::string ifname = line.substr(44);
    while(ifname.size() && ifname[0] == ' ')
      ifname = ifname.substr(1);
    if(isInterfaceBlacklisted(ifname))
      continue;

    ret.push_back(ip);
  }
}

/*
  Due to netlink system complexity we are using manually-built libnl wrapper
  library. Current implementation comes with a few limitations:
  - documentation for both netlink and libnl is let's say "not very good" and
  sometimes resembles sacred knowledge
  - our current build system has problems with one line in the socket.c file
    (see lib-config/libnl/socket.c.patch) causing sporadic illegal instruction
  exceptions when generating unique socket ports. They are caused by LLVM
  undefined behavior trap being triggered.
  - pthread rwlock in libnl sometimes causes undefined behavior on unlock on
  some systems when built with zig v0.9. Due to this we are using stl mutex. All
  netlink calls are done on library init, so this should not be a problem.
*/

// Pthread rwlock in libnl sometimes causes undefined behavior
// on unlock. This mutex should be used to protect all netlink calls.
std::mutex netlinkMutex;

static void linkEnableIPv6(const std::string& interface)
{
  int fd;
  std::string path("/proc/sys/net/ipv6/conf/" + interface + "/disable_ipv6");

  if((fd = open(path.c_str(), O_WRONLY)) < 0) {
    LOG_WARNING(
        "Unable to open %s (err: %s). Might be harmless.", path.c_str(),
        strerror(errno));
    return;
  }

  if(dprintf(fd, "0") < 0) {
    LOG_CRITICAL(
        "Unable to write to %s (err: %s). Might be harmless.", path.c_str(),
        strerror(errno));
  }

  if(close(fd) < 0) {
    LOG_CRITICAL(
        "Unable to close %s (err: %s). Might be harmless.", path.c_str(),
        strerror(errno));
  }
}

namespace Port {
  void init()
  {
    fatInit();

    if_inet6 = fopen("/proc/self/net/if_inet6", "r");

    if(if_inet6 == nullptr) {
      LOG_WARNING("failed to open if_inet6 file");
      return;
    }
  }

  void notifyReady()
  {
    const char* sockPath = getenv("NOTIFY_SOCKET");
    if(sockPath != NULL) {
      sockaddr_un un;
      un.sun_family = AF_UNIX;
      assert(strlen(sockPath) < sizeof(un.sun_path) - 1);

      memcpy(&un.sun_path[0], sockPath, strlen(sockPath) + 1);

      if(un.sun_path[0] == '@') {
        un.sun_path[0] = '\0';
      }

      int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
      if(fd < 0) {
        perror("systemd socket");
      }

      const char* msg = "READY=1";
      if(sendto(
             fd, msg, strlen(msg), MSG_NOSIGNAL, (sockaddr*)(&un),
             sizeof(un)) <= 0) {
        perror("systemd sendto");
      }

      if(close(fd) < 0) {
        perror("systemd close");
      }

      LOG_INFO("Systemd notification end");
    }
  }

  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName)
  {
    struct rtnl_link* link;
    struct nl_sock* ns;
    struct nl_cache* addr_cache;
    int err, ifindex;

    netlinkMutex.lock();

    // Setup netlink socket
    ns = nl_socket_alloc();
    if((err = nl_connect(ns, NETLINK_ROUTE)) < 0) {
      LOG_ERROR(
          "Unable to connect to netlink socket (err: %s)", nl_geterror(err));
      nl_socket_free(ns);
      return IpAddress();
    }

    // Get network interface by name
    if((err = rtnl_link_get_kernel(ns, 0, interfaceName.c_str(), &link)) < 0) {
      LOG_ERROR(
          "Unable to get interface %s information (err: %s)",
          interfaceName.c_str(), nl_geterror(err));
      nl_socket_free(ns);
      return IpAddress();
    }

    // Get interface addresses into cache
    if((err = rtnl_addr_alloc_cache(ns, &addr_cache)) < 0) {
      LOG_ERROR(
          "Unable to get interface %s addresses (err: %s)",
          interfaceName.c_str(), nl_geterror(err));
      nl_socket_free(ns);
      return IpAddress();
    }

    if((ifindex = rtnl_link_get_ifindex(link)) < 0) {
      LOG_ERROR(
          "Unable to get interface %s index (err: %s)", interfaceName.c_str(),
          nl_geterror(ifindex));
      nl_socket_free(ns);
      return IpAddress();
    }

    IpAddress ipv4{};
    IpAddress ipv6{};

    // Iterate over addresses in cache
    nl_object* obj = nl_cache_get_first(addr_cache);
    while(obj != NULL) {
      struct rtnl_addr* addr = (struct rtnl_addr*)obj;
      const char* addr_binary =
          (const char*)nl_addr_get_binary_addr(rtnl_addr_get_local(addr));
      int addr_family = rtnl_addr_get_family(addr);
      IpAddress addr_ip{};

      // Find IPv4 or IPv6 address for given interface
      if(rtnl_addr_get_ifindex(addr) == ifindex &&
         (addr_family == AF_INET || addr_family == AF_INET6)) {
        if(addr_family == AF_INET) {
          addr_ip = IpAddress::fromBinary4(addr_binary);

          if(!addr_ip.isReservedNotPrivate()) {
            ipv4 = addr_ip;
            break;  // Resolving interface address to IPv4 one is preferred
          }
        } else {  // v6
          addr_ip = IpAddress::fromBinary(addr_binary);

          if(addr_ip.isReservedNotPrivate())
            ipv6 = addr_ip;
          // We don't stop here, because we want to return IPv4 if possible
        }
      }

      // Get next address object from cache
      obj = nl_cache_get_next(obj);
    }

    // Cleanup
    nl_cache_free(addr_cache);
    rtnl_link_put(link);
    nl_socket_free(ns);

    netlinkMutex.unlock();

    if(ipv4) {
      return ipv4;
    } else {
      return ipv6;
    }
  }

  __attribute__((weak)) std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;

    getLocalIpv4Addresses(ret);
    getLocalIpv6Addresses(ret);

    return ret;
  }

  UpperLayer* startTunTap(
      const HusarnetAddress& myAddress,
      std::string interfaceName)
  {
    struct nl_sock* ns;
    struct rtnl_link* link;
    struct nl_addr* addr;
    struct rtnl_addr* link_addr;
    int err;

    netlinkMutex.lock();

    // Create TUN device if it doesn't exist
    if(access("/dev/net/tun", F_OK) == -1) {
      LOG_INFO("TUN device does not exist, creating it");

      mkdir("/dev/net", 0755);

      if(mknod("/dev/net/tun", S_IFCHR | 0666, makedev(10, 200)) == -1) {
        LOG_CRITICAL("Failed to create TUN device");
        exit(1);
      }
    }

    // Initialize TUN device
    auto tunTap = new TunTap(interfaceName);

    // Ensure that IPv6 is enabled on lo and TUN interfaces
    std::string loName = "lo";
    linkEnableIPv6(loName);
    linkEnableIPv6(interfaceName);

    // Setup netlink socket
    ns = nl_socket_alloc();
    if((err = nl_connect(ns, NETLINK_ROUTE)) < 0) {
      LOG_CRITICAL(
          "Failed to setup TUN device. Unable to connect to netlink socket "
          "(err: %s)",
          nl_geterror(err));
      nl_socket_free(ns);
      exit(1);
    }

    // Add a IP address to the TUN interface
    addr = nl_addr_build(AF_INET6, (void*)(myAddress.toBinary().c_str()), 16);
    nl_addr_set_prefixlen(addr, 16);

    link_addr = rtnl_addr_alloc();
    rtnl_addr_set_local(link_addr, addr);

    if((err = rtnl_link_get_kernel(ns, 0, interfaceName.c_str(), &link)) < 0) {
      LOG_CRITICAL(
          "Failed to setup TUN device. Unable to get interface %s information "
          "(err: %s)",
          interfaceName.c_str(), nl_geterror(err));

      rtnl_link_put(link);
      rtnl_addr_put(link_addr);
      nl_addr_put(addr);
      nl_socket_free(ns);

      exit(1);
    }

    rtnl_addr_set_link(link_addr, link);

    if((err = rtnl_addr_add(ns, link_addr, 0)) < 0) {
      LOG_CRITICAL(
          "Failed to setup TUN device. Unable to add address to interface %s "
          "(err: %s)",
          interfaceName.c_str(), nl_geterror(err));

      rtnl_link_put(link);
      rtnl_addr_put(link_addr);
      nl_addr_put(addr);
      nl_socket_free(ns);

      exit(1);
    }

    struct rtnl_link* change = rtnl_link_alloc();

    // Set MTU
    rtnl_link_set_mtu(change, 1350);

    // Set interface state to up
    rtnl_link_set_flags(change, IFF_UP);

    // Apply link changes, bring up interface
    if((err = rtnl_link_change(ns, link, change, 0)) < 0) {
      LOG_CRITICAL(
          "Failed to setup TUN device. Unable to apply link changes to "
          "interface %s (err: %s)",
          interfaceName.c_str(), nl_geterror(err));

      rtnl_link_put(change);
      rtnl_link_put(link);
      rtnl_addr_put(link_addr);
      nl_addr_put(addr);
      nl_socket_free(ns);

      exit(1);
    }

    rtnl_link_put(change);

    // Add multicast route
    nl_addr_put(addr);
    addr = nl_addr_build(AF_INET6, multicastDestination.toBinary().c_str(), 16);
    nl_addr_set_prefixlen(addr, 48);

    struct rtnl_route* route = rtnl_route_alloc();
    rtnl_route_set_family(route, AF_INET6);
    rtnl_route_set_scope(route, RT_SCOPE_UNIVERSE);
    rtnl_route_set_protocol(route, RTPROT_BOOT);
    rtnl_route_set_type(route, RTN_UNICAST);
    rtnl_route_set_dst(route, addr);
    rtnl_route_set_table(route, RT_TABLE_LOCAL);
    rtnl_route_set_iif(route, rtnl_link_get_ifindex(link));

    rtnl_nexthop* nh = rtnl_route_nh_alloc();
    rtnl_route_nh_set_ifindex(nh, rtnl_link_get_ifindex(link));
    rtnl_route_add_nexthop(route, nh);

    if((err = rtnl_route_add(ns, route, NLM_F_REPLACE)) < 0) {
      LOG_CRITICAL(
          "Failed to setup TUN device. Unable to add multicast route (err: %s)",
          nl_geterror(err));

      rtnl_route_put(route);
      rtnl_link_put(link);
      rtnl_addr_put(link_addr);
      nl_addr_put(addr);
      nl_socket_free(ns);

      exit(1);
    }

    rtnl_route_put(route);
    rtnl_link_put(link);
    rtnl_addr_put(link_addr);
    nl_addr_put(addr);
    nl_socket_free(ns);

    netlinkMutex.unlock();

    return tunTap;
  }
}  // namespace Port

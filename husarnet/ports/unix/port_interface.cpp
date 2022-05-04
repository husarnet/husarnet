// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port_interface.h"
#include <ares.h>
#include <thread>
#include "husarnet/network_dev.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/unix/tun.h"
#include "husarnet/util.h"

struct ares_result {
  int status;
  IpAddress address;
};

static void ares_wait(ares_channel channel)
{
  int nfds;
  fd_set readers, writers;
  struct timeval tv, *tvp;
  while(1) {
    FD_ZERO(&readers);
    FD_ZERO(&writers);
    nfds = ares_fds(channel, &readers, &writers);
    if(nfds == 0)
      break;
    tvp = ares_timeout(channel, NULL, &tv);
    select(nfds, &readers, &writers, NULL, tvp);
    ares_process(channel, &readers, &writers);
  }
}

static void ares_local_callback(
    void* arg,
    int status,
    int timeouts,
    struct ares_addrinfo* addrinfo)
{
  struct ares_result* result = (struct ares_result*)arg;
  result->status = status;

  if(status != ARES_SUCCESS) {
    LOG("DNS resolution failed. c-ares status code: %i (%s)", status,
        ares_strerror(status));
    return;
  }

  struct ares_addrinfo_node* node = addrinfo->nodes;
  while(node != NULL) {
    if(node->ai_family == AF_INET) {
      result->address = IpAddress::fromBinary4(
          reinterpret_cast<sockaddr_in*>(node->ai_addr)->sin_addr.s_addr);
    }
    if(node->ai_family == AF_INET6) {
      result->address = IpAddress::fromBinary(
          (const char*)reinterpret_cast<sockaddr_in6*>(node->ai_addr)
              ->sin6_addr.s6_addr);
    }

    node = node->ai_next;
  }

  ares_freeaddrinfo(addrinfo);
}

namespace Port {
  void init() { ares_library_init(ARES_LIB_INIT_NONE); }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    std::thread t(func);
    t.detach();
  }

  IpAddress resolveToIp(std::string hostname)
  {
    if(hostname.empty()) {
      LOG("Empty hostname provided for a DNS search");
      return IpAddress();
    }

    struct ares_result result;
    ares_channel channel;

    if(ares_init(&channel) != ARES_SUCCESS) {
      LOG("Unable to init ARES/DNS channel for doman: %s", hostname.c_str());
      return IpAddress();
    }

    struct ares_addrinfo_hints hints = {};
    hints.ai_flags |= ARES_AI_NUMERICSERV | ARES_AI_NOSORT;

    ares_getaddrinfo(
        channel, hostname.c_str(), "443", &hints, ares_local_callback,
        (void*)&result);
    ares_wait(channel);

    return result.address;
  }

  int64_t getCurrentTime()
  {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
  }

  // TODO this whole method should be rewritten *not* to utilize inline
  // bash(?!) and *to* utilize netlink interface
  void startTunTap(std::string name)
  {
    if(system("[ -e /dev/net/tun ] || (mkdir -p /dev/net; mknod /dev/net/tun c "
              "10 200)") != 0) {
      LOG("failed to create TUN device");
    }

    // TODO!
    // NgSocket* wrappedSock = sock;
    // NgSocket* netDev =
    //     NetworkDev::wrap(identity->deviceId, wrappedSock, [&](DeviceId id) {
    //       return configManager.getMulticastDestinations(id);
    //     });

    // std::string myIp = deviceIdToIpAddress(identity->deviceId).str();

    // TunDelegate::startTun(name, netDev);

    if(system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
       system(("sysctl net.ipv6.conf." + name + ".disable_ipv6=0").c_str()) !=
           0) {
      LOG("failed to enable IPv6 (may be harmless)");
    }

    // TODO!
    // if(system(("ip link set dev " + name + " mtu 1350").c_str()) != 0 ||
    //    system(("ip addr add dev " + name + " " + myIp + "/16").c_str()) != 0
    //    || system(("ip link set dev " + name + " up").c_str()) != 0) {
    //   LOG("failed to setup IP address");
    //   exit(1);
    // }

    // Multicast
    if(system(
           ("ip -6 route add ff15:f2d3:a389::/48 dev " + name + " table local")
               .c_str()) != 0) {
      LOG("failed to setup multicast route");
    }
  }
}  // namespace Port

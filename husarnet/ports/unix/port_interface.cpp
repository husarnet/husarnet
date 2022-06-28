// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port_interface.h"

#include <ares.h>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/unix/tun.h"

#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/util.h"

#include "enum.h"

class HigherLayer;

extern char** environ;

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
  void init()
  {
    signal(SIGPIPE, SIG_IGN);
    ares_library_init(ARES_LIB_INIT_NONE);
  }

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

  // TODO long term - this whole method should be rewritten *not* to utilize
  // inline bash(?!) and *to* utilize netlink interface
  HigherLayer* startTunTap(HusarnetManager* manager)
  {
    if(system("[ -e /dev/net/tun ] || (mkdir -p /dev/net; mknod /dev/net/tun c "
              "10 200)") != 0) {
      LOG("failed to create TUN device");
    }

    std::string myIp =
        deviceIdToIpAddress(manager->getIdentity()->getDeviceId()).str();

    auto interfaceName = manager->getInterfaceName();

    auto tunTap = new TunTap(interfaceName);

    if(system("sysctl net.ipv6.conf.lo.disable_ipv6=0") != 0 ||
       system(("sysctl net.ipv6.conf." + interfaceName + ".disable_ipv6=0")
                  .c_str()) != 0) {
      LOG("failed to enable IPv6 (may be harmless)");
    }

    if(system(("ip link set dev " + interfaceName + " mtu 1350").c_str()) !=
           0 ||
       system(
           ("ip addr add dev " + interfaceName + " " + myIp + "/16").c_str()) !=
           0 ||
       system(("ip link set dev " + interfaceName + " up").c_str()) != 0) {
      LOG("failed to setup IP address");
      exit(1);
    }

    // Multicast
    if(system(("ip -6 route add " + multicastDestination + "/48 dev " +
               interfaceName + " table local")
                  .c_str()) != 0) {
      LOG("failed to setup multicast route");
    }

    return tunTap;
  }

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    std::map<UserSetting, std::string> result;
    for(char** environ_ptr = environ; *environ_ptr != nullptr; environ_ptr++) {
      for(auto enumName : UserSetting::_names()) {
        auto candidate =
            "HUSARNET_" + strToUpper(camelCaseToUserscores(enumName));

        std::vector<std::string> splitted = split(*environ_ptr, '=', 1);
        if(splitted.size() == 1) {
          continue;
        }

        auto key = splitted[0];
        auto value = splitted[1];

        if(key == candidate) {
          result[UserSetting::_from_string(enumName)] = value;
          LOG("Overriding user setting %s=%s", enumName, value.c_str());
        }
      }
    }

    return result;
  }

  std::string readFile(std::string path)
  {
    std::ifstream f(path);
    if(!f.good()) {
      LOG("failed to open %s", path.c_str());
      exit(1);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }

  bool writeFile(std::string path, std::string data)
  {
    FILE* f = fopen((path + ".tmp").c_str(), "wb");
    int ret = fwrite(data.data(), data.size(), 1, f);
    if(ret != 1) {
      LOG("could not write to %s (failed to write to temporary file)",
          path.c_str());
      return false;
    }
    fsync(fileno(f));
    fclose(f);

    if(rename((path + ".tmp").c_str(), path.c_str()) < 0) {
      LOG("could not write to %s (rename failed)", path.c_str());
      return false;
    }
    return true;
  }

  bool isFile(std::string path) { return std::filesystem::exists(path); }

  void notifyReady()
  {
    const char* msg = "READY=1";
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
      assert(fd != 0);
      sendto(fd, msg, strlen(msg), MSG_NOSIGNAL, (sockaddr*)(&un), sizeof(un));
      close(fd);
    }
  }
}  // namespace Port

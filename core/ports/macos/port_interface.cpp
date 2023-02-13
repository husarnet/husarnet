// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port_interface.h"

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>

#include <ares.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/macos/tun.h"

#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "enum.h"

class UpperLayer;

extern char** environ;

struct ares_result {
  int status;
  IpAddress address;
};

static void ares_wait(ares_channel channel)
{
  fd_set readers, writers;

  while(true) {
    int nfds;
    struct timeval tv, *tvp;

    FD_ZERO(&readers);
    FD_ZERO(&writers);

    nfds = ares_fds(channel, &readers, &writers);
    if(nfds == 0) {
      break;
    }

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
    LOG_ERROR("DNS resolution failed. c-ares status code: %i (%s)", status,
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

  IpAddress resolveToIp(const std::string& hostname)
  {
    if(hostname.empty()) {
      LOG_ERROR("Empty hostname provided for a DNS search");
      return IpAddress();
    }

    struct ares_result result;
    ares_channel channel;

    if(ares_init(&channel) != ARES_SUCCESS) {
      LOG_ERROR("Unable to init ARES/DNS channel for domain: %s", hostname.c_str());
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

  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    std::string myIp =
        deviceIdToIpAddress(manager->getIdentity()->getDeviceId()).str();

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

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    std::map<UserSetting, std::string> result;
    for(char** environ_ptr = environ; *environ_ptr != nullptr; environ_ptr++) {
      for(auto enumName : UserSetting::_names()) {
        auto candidate =
            "HUSARNET_" + strToUpper(camelCaseToUnderscores(enumName));

        std::vector<std::string> splitted = split(*environ_ptr, '=', 1);
        if(splitted.size() == 1) {
          continue;
        }

        auto key = splitted[0];
        auto value = splitted[1];

        if(key == candidate) {
          result[UserSetting::_from_string(enumName)] = value;
          LOG_WARNING("Overriding user setting %s=%s", enumName, value.c_str());
        }
      }
    }

    return result;
  }

  std::string readFile(const std::string& path)
  {
    std::ifstream f(path);
    if(!f.good()) {
      LOG_ERROR("failed to open %s", path.c_str());
      exit(1);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }

  static bool writeFileDirect(const std::string& path, const std::string& data)
  {
    FILE* f = fopen(path.c_str(), "wb");
    int ret = fwrite(data.data(), data.size(), 1, f);
    fsync(fileno(f));
    fclose(f);

    if(ret != 1) {
      return false;
    }

    return true;
  }

  static bool removeFile(const std::string& path)
  {
    if(remove(path.c_str()) != 0) {
      return false;
    }

    return true;
  }

  static bool
  renameFileReal(const std::string& src, const std::string& dst, bool quiet)
  {
    bool success = rename(src.c_str(), dst.c_str()) == 0;
    if(!success) {
      if(!quiet) {
        LOG_WARNING("failed to rename %s to %s", src.c_str(), dst.c_str());
      }
      return false;
    }

    return true;
  }

  bool writeFile(const std::string& path, const std::string& data)
  {
    std::string tmpPath = path + ".tmp";

    bool success = writeFileDirect(tmpPath, data);
    if(!success) {
      LOG_INFO(
          "unable to write to a temporary file %s, writing to %s directly",
          tmpPath.c_str(), path.c_str());
      success = writeFileDirect(path, data);
      if(!success) {
        LOG_WARNING("unable to write to %s directly", path.c_str());
        return false;
      }
      return true;
    }

    success = renameFileReal(tmpPath, path, true);
    if(success) {
      return true;
    }

    LOG_DEBUG(
        "unable to rename %s to %s, writing to %s directly", tmpPath.c_str(),
        path.c_str(), path.c_str());

    success = removeFile(tmpPath);
    if(!success) {
      LOG_INFO("unable to remove temporary file %s", tmpPath.c_str());
    }

    success = writeFileDirect(path, data);
    if(!success) {
      LOG_WARNING("unable to write directly to %s", path.c_str());
      return false;
    }

    return true;
  }

  bool isFile(const std::string& path)
  {
    return std::filesystem::exists(path);
  }

  bool renameFile(const std::string& src, const std::string& dst)
  {
    return renameFileReal(src, dst, false);
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

      LOG_WARNING("Systemd notification end");
    }
  }

  void log(const std::string& message)
  {
    fprintf(stderr, "%s\n", message.c_str());
    fflush(stderr);
  }
}  // namespace Port

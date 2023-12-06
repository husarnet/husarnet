// Copyright (c) 2024 Husarnet sp. z o.o.
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
#include <dirent.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/route.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/linux/tun.h"

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

// Pthread rwlock in libnl sometimes causes undefined behavior
// on unlock. This mutex should be used to protect all netlink calls.
std::mutex netlinkMutex;

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
    LOG_ERROR(
        "DNS resolution failed. c-ares status code: %i (%s)", status,
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
    new std::thread([name, func]() {
      try {
        func();
      } catch(const std::exception& exc) {
        LOG_CRITICAL("unhandled exception in thread %s: %s", name, exc.what());
      } catch(const std::string& exc) {
        LOG_CRITICAL("unhandled exception in thread %s: %s", name, exc.c_str());
      } catch(...) {
        LOG_CRITICAL("unknown and unhandled exception in thread %s", name);
      }
    });
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
      LOG_ERROR(
          "Unable to init ARES/DNS channel for domain: %s", hostname.c_str());
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
            break;  // Resolving interface address to IPv4 one is prefered
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

  int64_t getCurrentTime()
  {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
  }

  static void linkEnableIPv6(std::string& interface)
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

  // TODO long term - this whole method should be rewritten *not* to utilize
  // inline bash(?!) and *to* utilize netlink interface
  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    struct nl_sock* ns;
    struct rtnl_link* link;
    struct nl_addr* addr;
    struct rtnl_addr* link_addr;
    int err;

    IpAddress myIp = deviceIdToIpAddress(manager->getIdentity()->getDeviceId());
    auto interfaceName = manager->getInterfaceName();

    netlinkMutex.lock();

    // Create TUN device if it doesn't exist
    if(system("[ -e /dev/net/tun ] || (mkdir -p /dev/net; mknod /dev/net/tun c "
              "10 200)") != 0) {
      LOG_CRITICAL("Failed to create TUN device");
      exit(1);
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
    addr = nl_addr_build(AF_INET6, (void*)myIp.toBinary().c_str(), 16);
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

    if((err = rtnl_route_add(ns, route, NLM_F_REPLACE) < 0)) {
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

  static bool writeFileDirect(const std::string& path, const std::string& data)
  {
    FILE* f = fopen(path.c_str(), "wb");
    if(f == nullptr) {
      throw std::runtime_error{
          "Error: \"" + std::string{strerror(errno)} +
          "\" while opening file: \"" + path + "\""};
    }
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
      LOG_WARNING(
          "unable to write to a temporary file %s, writing to %s directly",
          tmpPath.c_str(), path.c_str());
      success = writeFileDirect(path, data);
      if(!success) {
        LOG_ERROR("unable to write to %s directly", path.c_str());
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
      LOG_WARNING("unable to remove temporary file %s", tmpPath.c_str());
    }

    success = writeFileDirect(path, data);
    if(!success) {
      LOG_ERROR("unable to write directly to %s", path.c_str());
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

      LOG_INFO("Systemd notification end");
    }
  }

  void log(const std::string& message)
  {
    fprintf(stderr, "%s\n", message.c_str());
    fflush(stderr);
  }

  static std::list<std::string> listScripts(const std::string& path)
  {
    std::list<std::string> result;
    DIR* dir = opendir(path.c_str());
    if(dir == NULL) {
      return result;
    }

    struct dirent* ent;
    while((ent = readdir(dir)) != NULL) {
      std::string fileName = ent->d_name;
      if(fileName == "." || fileName == "..") {
        continue;
      }

      std::string filePath = path + "/" + fileName;
      if(access(filePath.c_str(), X_OK) == 0) {
        result.push_back(filePath);
      }
    }
    closedir(dir);

    return result;
  }

  void runScripts(const std::string& path)
  {
    auto scriptPaths = listScripts(path);
    if(scriptPaths.empty()) {
      return;
    }

    LOG_INFO(("running hooks under path " + path).c_str());
    for(auto& scriptPath : scriptPaths) {
      pid_t pid = fork();
      if(pid == 0) {
        LOG_INFO(("running " + scriptPath + " as a hook").c_str());
        std::system((char*)scriptPath.c_str());
      } else {
        int status;
        waitpid(pid, &status, 0);
      }
    }
  }

  bool checkScriptsExist(const std::string& path)
  {
    LOG_DEBUG(("checking for valid hooks under " + path).c_str());
    auto scriptPaths = listScripts(path);
    return !scriptPaths.empty();
  }
}  // namespace Port

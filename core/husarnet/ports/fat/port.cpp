// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port.h"

#include <chrono>
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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "husarnet/ports/fat/filesystem.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "magic_enum/magic_enum_all.hpp"

#ifndef PORT_WINDOWS
extern char** environ;
#endif

const static std::string hostnamePath = "/etc/hostname";

#ifdef PORT_WINDOWS
const static std::string configDir = std::string(getenv("PROGRAMDATA")) + "\\Husarnet\\";
const static std::string filesDir = configDir + "files\\";
#else
const static std::string configDir = "/var/lib/husarnet/";
const static std::string filesDir = configDir + "files/";
#endif

#include "httplib.h"

static bool validateHostname(std::string hostname)
{
  if(hostname.size() == 0)
    return false;
  for(char c : hostname) {
    bool ok = ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') ||
              (c == '_' || c == '-' || c == '.');
    if(!ok) {
      return false;
    }
  }
  return true;
}

static std::string updateHostsFileInternal(std::map<std::string, IpAddress> data, std::string oldContent)
{
  const std::string marker = " # managed by Husarnet";

#ifdef PORT_WINDOWS
  const std::string lineEnding = "\r\n";
#else
  const std::string lineEnding = "\n";
#endif

  auto hostsFileStream = std::istringstream(oldContent);
  std::string newContent = "";

  // First rewrite all unmanaged lines as they are
  std::string line;
  while(std::getline(hostsFileStream, line)) {
    // getline assumes line ending is LF by default, even on Windows
    // in earlier versions of Husarnet hosts file on Windows got unwanted orphan
    // CRs here we make sure no such thing happen again AND we clean up after
    // this bug we could just add delimiter param to std::getline above, but we
    // still have to clean up after older versions so might as well leave
    // getline as-is and just trim excess ws.
    line = rtrim(line);
    if(!endsWith(line, marker)) {
      newContent += line + lineEnding;
    }
  }

  // Then add our lines
  for(auto& [hostname, address] : data) {
    if(!validateHostname(hostname)) {
      continue;
    }

    newContent += address.toString() + " " + hostname + marker + lineEnding;
  }

  return newContent;
}

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

static void ares_local_callback(void* arg, int status, int timeouts, struct ares_addrinfo* addrinfo)
{
  struct ares_result* result = (struct ares_result*)arg;
  result->status = status;

  if(status != ARES_SUCCESS) {
    LOG_ERROR("DNS resolution failed. c-ares status code: %i (%s)", status, ares_strerror(status));
    return;
  }

  struct ares_addrinfo_node* node = addrinfo->nodes;
  while(node != NULL) {
    if(node->ai_family == AF_INET) {
      result->address = IpAddress::fromBinary4(reinterpret_cast<sockaddr_in*>(node->ai_addr)->sin_addr.s_addr);
    }
    if(node->ai_family == AF_INET6) {
      result->address =
          IpAddress::fromBinary((const char*)reinterpret_cast<sockaddr_in6*>(node->ai_addr)->sin6_addr.s6_addr);
    }

    node = node->ai_next;
  }

  ares_freeaddrinfo(addrinfo);
}

namespace Port {
  void fatInit()
  {
#ifndef PORT_WINDOWS
    signal(SIGPIPE, SIG_IGN);
#endif

    ares_library_init(ARES_LIB_INIT_NONE);

    if(!std::filesystem::exists(configDir)) {
      try {
        std::filesystem::create_directory(configDir);
      } catch(const std::exception& e) {
        LOG_WARNING("Unable to create config directory: %s", e.what());
      }
    }
  }

  __attribute__((weak)) void threadStart(std::function<void()> func, const char* name, int stack, int priority)
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

  __attribute__((weak)) void threadSleep(Time ms)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  static etl::map<std::string, EnvKey, ENV_KEY_OPTIONS> envMap = {
      etl::pair{std::string("HUSARNET_INSTANCE_FQDN"), EnvKey::tldFqdn},
      etl::pair{std::string("HUSARNET_LOG_VERBOSITY"), EnvKey::logVerbosity},
      etl::pair{std::string("HUSARNET_ENABLE_HOOKS"), EnvKey::enableHooks},
      etl::pair{std::string("HUSARNET_ENABLE_CONTROLPLANE"), EnvKey::enableControlPlane},
      etl::pair{std::string("HUSARNET_DAEMON_INTERFACE"), EnvKey::daemonInterface},
      etl::pair{std::string("HUSARNET_DAEMON_API_INTERFACE"), EnvKey::daemonApiInterface},
      etl::pair{std::string("HUSARNET_DAEMON_API_HOST"), EnvKey::daemonApiHost},
      etl::pair{std::string("HUSARNET_DAEMON_API_PORT"), EnvKey::daemonApiPort},
  };

  __attribute__((weak)) etl::map<EnvKey, std::string, ENV_KEY_OPTIONS> getEnvironmentOverrides()
  {
    etl::map<EnvKey, std::string, ENV_KEY_OPTIONS> result;
    for(char** environ_ptr = environ; *environ_ptr != nullptr; environ_ptr++) {
      std::vector<std::string> splitted = split(*environ_ptr, '=', 1);
      if(splitted.size() == 1) {
        continue;
      }

      auto key = strToUpper(splitted[0]);
      auto value = splitted[1];

      if(envMap.contains(key)) {
        result[envMap[key]] = value;
      }
    }

    return result;
  }

  __attribute__((weak)) void log(const LogLevel level, const std::string& message)
  {
    fprintf(stderr, "%s\n", message.c_str());
    fflush(stderr);
  }

  __attribute__((weak)) int64_t getCurrentTime()
  {
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return ms.count();
  }

  __attribute__((weak)) const std::string getHumanTime()
  {
    auto now = std::chrono::system_clock::now();

    std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm localtime = *std::localtime(&timestamp);

    return std::to_string(localtime.tm_year + 1900) + "-" + padLeft(2, std::to_string(localtime.tm_mon + 1), '0') +
           "-" + padLeft(2, std::to_string(localtime.tm_mday), '0') + "_" +
           padLeft(2, std::to_string(localtime.tm_hour), '0') + ":" +
           padLeft(2, std::to_string(localtime.tm_min), '0') + ":" + padLeft(2, std::to_string(localtime.tm_sec), '0');
  }

  __attribute__((weak)) void processSocketEvents(void* tuntap)
  {
    OsSocket::runOnce(1000);  // process socket events for at most so many ms
  }

  __attribute__((weak)) std::string getSelfHostname()
  {
#ifndef PORT_WINDOWS
    auto hostname = readFile(hostnamePath);

    if(!hostname.empty())
      return rtrim(hostname);

    // On some platforms (i.e. OpenWRT) the hostname file does not exist
    LOG_WARNING(
        "hostname file does not exist on this system, deriving one from the "
        "host id");

    // Use unique hostid as a hostname
    long hostid = gethostid();
    std::vector<unsigned char> hostid_bytes = {
        (unsigned char)(hostid >> 24),
        (unsigned char)(hostid >> 16),
        (unsigned char)(hostid >> 8),
        (unsigned char)(hostid >> 0),
    };

    std::string hostid_str = encodeHex(hostid_bytes);

    return "device-" + hostid_str;
#else
    return "windows-has-its-own-way";
#endif
  }

  __attribute__((weak)) bool setSelfHostname(const std::string& newHostname)
  {
    if(!validateHostname(newHostname)) {
      return false;
    }

    // If hostname is already set to the new one, don't re-write it
    if(newHostname == getSelfHostname()) {
      return true;
    }

    // Using transform instead of simple write as it's writing to /etc and
    // transform has already all the necessary mechanisms for making it safe
    transformFile(hostnamePath, [newHostname](const std::string& oldContent) { return newHostname; });

    if(system("hostname -F /etc/hostname") != 0) {
      LOG_ERROR("cannot update hostname to %s", newHostname.c_str());
      return false;
    }

    return true;
  }

  __attribute__((weak)) void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
#ifdef PORT_WINDOWS
    const std::string hostsFilePath = std::string(getenv("windir")) + "/system32/drivers/etc/hosts";
#else
    const std::string hostsFilePath = "/etc/hosts";
#endif

    transformFile(
        hostsFilePath, [data](const std::string& oldContent) { return updateHostsFileInternal(data, oldContent); });
  }

  __attribute__((weak)) IpAddress resolveToIp(const std::string& hostname)
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

    ares_getaddrinfo(channel, hostname.c_str(), "443", &hints, ares_local_callback, (void*)&result);
    ares_wait(channel);

    LOG_DEBUG("DNS resolution for %s done, result: %s", hostname.c_str(), result.address.toString().c_str());

    return result.address;
  }

  __attribute__((weak)) bool runHook(HookType hookType)
  {
    auto hookName = std::string(magic_enum::enum_name(hookType));
    auto path = filesDir + "hook_" + hookName;

    if(access(path.c_str(), X_OK) != 0) {
      LOG_INFO(("hook " + path + " not found or not executable").c_str());
      return false;
    }

    Port::threadStart(
        [path, hookName]() {
          // TODO probably can be replaced with execve or similar
          LOG_DEBUG(("running " + hookName).c_str());

          FILE* pipe = popen((path + " 2>&1").c_str(), "r");
          if(!pipe) {
            LOG_ERROR(("failed to run " + path).c_str());
            return;
          }

          char buffer[1024];
          std::string output;

          // nullptr here is a delimiter of subprocess end
          while(fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
          }

          std::istringstream iss(output);
          std::string line;
          while(std::getline(iss, line)) {
            LOG_INFO((hookName + ": " + line).c_str());
          }

          auto ret = pclose(pipe);

          LOG_INFO((hookName + " finished with exit code: " + std::to_string(ret)).c_str());
        },
        hookName.c_str());

    return true;
  }

  static const etl::map<StorageKey, std::string, STORAGE_KEY_OPTIONS> storageMap = {
      etl::pair{StorageKey::id, std::string("id")},
      etl::pair{StorageKey::config, std::string("config.json")},
      etl::pair{StorageKey::daemonApiToken, std::string("daemon_api_token")},
      etl::pair{StorageKey::cache, std::string("cache.json")},
      etl::pair{StorageKey::windowsDeviceGuid, std::string("guid")},
  };

  __attribute__((weak)) std::string readStorage(StorageKey key)
  {
    auto path = configDir + storageMap.at(key);
    return readFile(path);
  }

  __attribute__((weak)) bool writeStorage(StorageKey key, const std::string& data)
  {
    auto path = configDir + storageMap.at(key);
    return writeFile(path, data);
  }

  __attribute__((weak)) HttpResult httpGet(const std::string& host, const std::string& path)
  {
    httplib::Client httpClient(host);
    auto result = httpClient.Get(path);

    if(result) {
      return {result->status, result->body};
    } else {
      auto err = result.error();
      LOG_ERROR("Can't contact host %s (error: %s)", host.c_str(), httplib::to_string(err).c_str());
    }
    return {};
  }

  __attribute__((weak)) HttpResult httpGet(const IpAddress& ip, const std::string& path)
  {
    return Port::httpGet(ip.toString(), path);
  }

  __attribute__((weak)) HttpResult httpPost(const std::string& host, const std::string& path, const std::string& body)
  {
    httplib::Client httpClient(host);
    auto result = httpClient.Post(path, body, "application/json");

    if(result) {
      return {result->status, result->body};
    } else {
      auto err = result.error();
      LOG_ERROR("Can't contact host %s (error: %s)", host.c_str(), httplib::to_string(err).c_str());
    }
    return {};
  }

  HttpResult httpPost(const IpAddress& ip, const std::string& path, const std::string& body)
  {
    return Port::httpPost(ip.toString(), path, body);
  }
}  // namespace Port

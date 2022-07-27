// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <chrono>
#include <fstream>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/ports/threads_port.h"
#include "husarnet/ports/windows/tun.h"

#include "husarnet/gil.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/util.h"

#include "shlwapi.h"

// Based on code from libtuntap (https://github.com/LaKabane/libtuntap, ISC
// License)
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define NETWORK_ADAPTERS                                                 \
  "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-" \
  "08002BE10318}"

// TODO change later ympek
bool husarnetVerbose = true;

static std::vector<std::string> getExistingDeviceNames()
{
  std::vector<std::string> names;
  const char* key_name = NETWORK_ADAPTERS;
  HKEY adapters, adapter;
  DWORD i, ret, len;
  DWORD sub_keys = 0;

  ret =
      RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(key_name), 0, KEY_READ, &adapters);
  if(ret != ERROR_SUCCESS) {
    LOG("RegOpenKeyEx returned error");
    return {};
  }

  ret = RegQueryInfoKey(
      adapters, NULL, NULL, NULL, &sub_keys, NULL, NULL, NULL, NULL, NULL, NULL,
      NULL);
  if(ret != ERROR_SUCCESS) {
    LOG("RegQueryInfoKey returned error");
    return {};
  }

  if(sub_keys <= 0) {
    LOG("Wrong registry key");
    return {};
  }

  /* Walk througt all adapters */
  for(i = 0; i < sub_keys; i++) {
    char new_key[MAX_KEY_LENGTH];
    char data[256];
    TCHAR key[MAX_KEY_LENGTH];
    DWORD keylen = MAX_KEY_LENGTH;

    /* Get the adapter key name */
    ret = RegEnumKeyEx(adapters, i, key, &keylen, NULL, NULL, NULL, NULL);
    if(ret != ERROR_SUCCESS) {
      continue;
    }

    /* Append it to NETWORK_ADAPTERS and open it */
    snprintf(new_key, sizeof new_key, "%s\\%s", key_name, key);
    ret =
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT(new_key), 0, KEY_READ, &adapter);
    if(ret != ERROR_SUCCESS) {
      continue;
    }

    /* Check its values */
    len = sizeof data;
    ret =
        RegQueryValueEx(adapter, "ComponentId", NULL, NULL, (LPBYTE)data, &len);
    if(ret != ERROR_SUCCESS) {
      /* This value doesn't exist in this adaptater tree */
      goto clean;
    }
    /* If its a tap adapter, its all good */
    if(strncmp(data, "tap", 3) == 0) {
      DWORD type;

      len = sizeof data;
      ret = RegQueryValueEx(
          adapter, "NetCfgInstanceId", NULL, &type, (LPBYTE)data, &len);
      if(ret != ERROR_SUCCESS) {
        LOG("RegQueryValueEx returned error");
        goto clean;
      }
      LOG("found tap device: %s", data);

      names.push_back(std::string(data));
      // break;
    }
  clean:
    RegCloseKey(adapter);
  }
  RegCloseKey(adapters);
  return names;
}

static std::string whatNewDeviceWasCreated(
    std::vector<std::string> previousNames)
{
  for(std::string name : getExistingDeviceNames()) {
    if(std::find(previousNames.begin(), previousNames.end(), name) ==
       previousNames.end()) {
      LOG("new device: %s", name.c_str());
      return name;
    }
  }
  LOG("no new device was created?");
  abort();
}

namespace Port {
  thread_local const char* threadName = nullptr;

  void runThread(void* arg)
  {
    auto f = (std::pair<char*, std::function<void()>>*)arg;
    threadName = f->first;
    f->second();
  }

  void init()
  {
    // GIL is inited on (stage1)
    // GIL::init();
    WSADATA wsaData;
    WSAStartup(0x202, &wsaData);
  }

  const char* getThreadName()
  {
    return threadName ? threadName : "null";
  }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    auto* f = new std::pair<const char*, std::function<void()>>(
        name, std::move(func));
    _beginthread(runThread, 0, f);
  }

  IpAddress resolveToIp(std::string hostname)
  {
    // we are using raw getaddrinfo, not ares
    // this might be not cool
    // but also it might work
    struct addrinfo* result = nullptr;
    int error;

    error = SOCKFUNC(getaddrinfo)(hostname.c_str(), "443", NULL, &result);
    if(error != 0) {
      return IpAddress();
    }

    for(struct addrinfo* res = result; res != NULL; res = res->ai_next) {
      if(res->ai_family == AF_INET || res->ai_family == AF_INET6) {
        sockaddr_storage ss{};
        ss.ss_family = res->ai_family;
        assert(sizeof(sockaddr_storage) >= res->ai_addrlen);
        memcpy(&ss, res->ai_addr, res->ai_addrlen);
        SOCKFUNC(freeaddrinfo)(result);
        return OsSocket::ipFromSockaddr(ss).ip;
      }
    }

    SOCKFUNC(freeaddrinfo)(result);

    return IpAddress();
  }

  int64_t getCurrentTime()
  {
    using namespace std::chrono;
    milliseconds ms =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return ms.count();
  }

  HigherLayer* startTunTap(HusarnetManager* manager)
  {
    auto existingDevices = getExistingDeviceNames();
    auto deviceName = manager->getInterfaceName();
    // this should also work if deviceName is ""
    LOG("WINDOWS DEVICE NAME is %s", deviceName.c_str());
    if(std::find(existingDevices.begin(), existingDevices.end(), deviceName) ==
       existingDevices.end()) {
      system("addtap.bat");
      auto newDeviceName = whatNewDeviceWasCreated(existingDevices);
      manager->setInterfaceName(newDeviceName);
    }

    auto interfaceName = manager->getInterfaceName();
    auto tunTap = new TunTap(interfaceName);

    return tunTap;
  }

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    std::map<UserSetting, std::string> result;

    std::string envVarBuffer;
    envVarBuffer.resize(512);

    for(auto enumName : UserSetting::_names()) {
      auto candidate =
          "HUSARNET_" + strToUpper(camelCaseToUserscores(enumName));

      DWORD envVarLength = GetEnvironmentVariable(
          candidate.c_str(), &envVarBuffer[0], (DWORD)envVarBuffer.size());
      if(GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
        continue;
      }

      std::string value(&envVarBuffer[0], envVarLength);
      result[UserSetting::_from_string(enumName)] = value;
      LOG("Overriding user setting %s=%s", enumName, value.c_str());
    }

    return result;
  }

  std::string readFile(std::string path)
  {
    // ympek TODO
    // copied from Unix implementation
    // candidate for ports_common.cpp or something
    std::ifstream f(path);
    if(!f.good()) {
      LOG("failed to open %s", path.c_str());
      exit(1);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }

  bool writeFile(std::string path, std::string content)
  {
    std::ofstream f(path, std::ofstream::out);
    if(!f.good()) {
      LOG("failed to write: %s", path.c_str());
      // hmm
      f.close();
      return false;
    }
    f << content;
    f.close();
    return true;
  }

  bool isFile(std::string path)
  {
    return PathFileExists(path.c_str());
  }

  void notifyReady()
  {
  }
}  // namespace Port

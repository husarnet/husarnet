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

#include <arpa/inet.h>
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

#include "husarnet/config_storage.h"
#include "husarnet/device_id.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "enum.h"
#include "esp_system.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs.h"
#include "nvs_flash.h"

namespace Port {
  static nvs_handle nvsHandle;

  void init()
  {
    int ok = nvs_open("husarnet", NVS_READWRITE, &nvsHandle);
    if(ok != ESP_OK) {
      LOG("Unable to access non volatile memory. This will result in corrupted "
          "operations!");
    }
  }

  static void taskProc(void* p)
  {
    std::function<void()>* func1 = (std::function<void()>*)p;
    (*func1)();
    delete func1;
    vTaskDelete(NULL);
  }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    TaskHandle_t handle;
    std::function<void()>* func1 = new std::function<void()>;
    *func1 = std::move(func);
    int coreId = strcmp(name, "ngsocket") == 0 ? 1 : 0;
    if(xTaskCreatePinnedToCore(
           taskProc, name, stack, func1, priority, &handle, coreId) != pdTRUE) {
      abort();
    }
  }

  IpAddress resolveToIp(const std::string& hostname)
  {
    struct addrinfo* result = nullptr;
    int error;

    error = getaddrinfo(hostname.c_str(), "", NULL, &result);
    if(error != 0) {
      return IpAddress();
    }

    for(struct addrinfo* res = result; res != NULL; res = res->ai_next) {
      if(res->ai_family == AF_INET) {
        auto addr = IpAddress::fromBinary4(res->ai_addr->sa_data);
        freeaddrinfo(result);
        return addr;
      }
      if(res->ai_family == AF_INET6) {
        auto addr = IpAddress::fromBinary(res->ai_addr->sa_data);
        freeaddrinfo(result);
        return addr;
      }
    }

    freeaddrinfo(result);

    return IpAddress();
  }

  int64_t getCurrentTime()
  {
    return esp_timer_get_time() / 1000 + 10000000ul;
  }

  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    return NULL;  // @TODO
  }

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    return std::map<UserSetting, std::string>();  // @TODO
  }

  std::string readFile(const std::string& path)
  {
    size_t requiredSize = 0;
    int ok = nvs_get_blob(nvsHandle, path.c_str(), NULL, &requiredSize);
    if(ok != ESP_OK) {
      LOG("Unable to access non volatile memory. This will result in "
          "corrupted "
          "operations!");
      return "";
    }
    std::string s;
    s.resize(requiredSize);
    ok = nvs_get_blob(nvsHandle, path.c_str(), &s[0], &requiredSize);
    if(ok != ESP_OK) {
      LOG("Unable to access non volatile memory. This will result in "
          "corrupted "
          "operations!");
      return "";
    }
    return s;
  }

  bool writeFile(const std::string& path, const std::string& data)
  {
    LOG("write %s (len: %d)", path.c_str(), (int)data.size());

    if(nvs_set_blob(nvsHandle, path.c_str(), &data[0], data.size()) != ESP_OK) {
      LOG("failed to update key %s", path.c_str());
      return false;
    }

    if(nvs_commit(nvsHandle) != ESP_OK) {
      LOG("failed to commit key %s", path.c_str());
      return false;
    }

    return true;
  }

  bool isFile(const std::string& path)
  {
    return false;  // @TODO
  }

  bool renameFile(const std::string& src, const std::string& dst)
  {
    return false;  // @TODO
  }

  void notifyReady()
  {
    // No-op as we won't be using it on this platform
  }

  void log(const std::string& message)
  {
    // @TODO
  }
}  // namespace Port

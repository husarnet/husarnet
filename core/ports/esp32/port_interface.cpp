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

#include "husarnet/ports/esp32/tun.h"
#include "husarnet/ports/sockets.h"

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
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"

#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

static const char* LOG_TAG = "husarnet";

namespace Port {
  std::unique_ptr<nvs::NVSHandle> nvsHandle;

  void init()
  {
    esp_err_t err;
    nvsHandle = nvs::open_nvs_handle("husarnet", NVS_READWRITE, &err);

    if(err != ESP_OK) {
      LOG_ERROR("Unable to open NVS. This will result in corrupted "
          "operations! (Error: %s)", esp_err_to_name(err));
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
    std::function<void()>* func1 = new std::function<void()>;
    *func1 = std::move(func);
    // int coreId = strcmp(name, "ngsocket") == 0 ? 1 : 0; //TODO: refactor
    // if(xTaskCreatePinnedToCore(
    //        taskProc, name, stack, func1, priority, &handle, coreId) != pdTRUE) {
    //   abort();
    // }
    if (xTaskCreate(taskProc, name, stack, func1, priority, NULL) != pdTRUE) {
      abort();
    }
  }

  IpAddress resolveToIp(const std::string& hostname)
  {
    struct addrinfo* result = nullptr;
    int error;

    error = getaddrinfo(hostname.c_str(), NULL, NULL, &result);
    if(error != 0) {
      return IpAddress();
    }

    for(struct addrinfo* res = result; res != NULL; res = res->ai_next) {
      if(res->ai_family == AF_INET) {
        auto addr = IpAddress::fromBinary4((uint32_t)((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(result);
        return addr;
      }
      if(res->ai_family == AF_INET6) {
        auto addr = IpAddress::fromBinary((char*)((struct sockaddr_in6*)res->ai_addr)->sin6_addr.s6_addr);
        freeaddrinfo(result);
        return addr;
      }
    }

    freeaddrinfo(result);

    return IpAddress();
  }

  int64_t getCurrentTime()
  {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
  }

  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    ip6_addr_t ip;
    memcpy(ip.addr, manager->getIdentity()->getDeviceId().data(), 16);
    
    auto tunTap = new TunTap(ip, 32);
    return tunTap;
  }

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    auto map = std::map<UserSetting, std::string>();
    map.insert({UserSetting::joinCode, "xxxx"}); //TODO: create join api
    map.insert({UserSetting::logVerbosity, "3"});
    
    return map;// @TODO
  }

  std::string readFile(const std::string& path)
  {
    size_t len;    
    esp_err_t err = nvsHandle->get_item_size(nvs::ItemType::SZ, path.c_str(), len);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
      return "";
    }

    if(err != ESP_OK) {
      LOG_ERROR("Unable to access NVS. (Error: %s)", esp_err_to_name(err));
      return "";
    }

    std::string value;
    value.resize(len);

    err = nvsHandle->get_string(path.c_str(), value.data(), len);

    if(err != ESP_OK) {
      LOG_ERROR("Unable to access NVS. (Error: %s)", esp_err_to_name(err));
      return "";
    }

    return value;
  }

  bool writeFile(const std::string& path, const std::string& data)
  {
    LOG("write %s (len: %d)", path.c_str(), (int)data.size());

    esp_err_t err = nvsHandle->set_string(path.c_str(), data.c_str());
    if (err != ESP_OK) {
      LOG_ERROR("Unable to update NVS. (Error: %s)", esp_err_to_name(err));
      return false;
    }

    if(nvsHandle->commit() != ESP_OK) {
      LOG_ERROR("Unable to commit NVS. (Error: %s)", esp_err_to_name(err));
      return false;
    }

    return true;
  }

  bool isFile(const std::string& path)
  {
    size_t value;
    esp_err_t err = nvsHandle->get_item_size(nvs::ItemType::SZ, path.c_str(), value);

    if (err == ESP_OK) {
      return true;
    }

    if (err == ESP_ERR_NVS_NOT_FOUND) {
      return false;
    }

    LOG_ERROR("Unable to access NVS. (Error: %s)", esp_err_to_name(err));
    return false;
  }

  bool renameFile(const std::string& src, const std::string& dst)
  {
    LOG_ERROR("renameFile not implemented");
    return false;  // @TODO
  }

  void notifyReady()
  {
    // No-op as we won't be using it on this platform
  }

  void log(const LogLevel level, const std::string& message)
  {
    esp_log_level_t esp_level;

    switch (level)
    {
      case +LogLevel::DEBUG:
        esp_level = ESP_LOG_DEBUG;
        break;
      case +LogLevel::INFO:
        esp_level = ESP_LOG_INFO;
        break;
      case +LogLevel::WARNING:
        esp_level = ESP_LOG_WARN;
        break;
      case +LogLevel::ERROR:
        esp_level = ESP_LOG_ERROR;
        break;
      case +LogLevel::CRITICAL:
        esp_level = ESP_LOG_ERROR;
        break;
      default:
        esp_level = ESP_LOG_INFO;
        break;
    }

    ESP_LOG_LEVEL(esp_level, LOG_TAG, "%s", message.c_str());
  }

  const std::string getHumanTime() {
    return "NOT IMPLEMENTED"; //TODO: implement
  }

  void processSocketEvents(HusarnetManager* manager)
  {
    OsSocket::runOnce(10);  // process socket events for at most so many ms
    manager->getTunTap()->processQueuedPackets();
  }

  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName)
  {
    LOG_ERROR("getIpAddressFromInterfaceName is not implemented");
    return IpAddress();
  }
}  // namespace Port

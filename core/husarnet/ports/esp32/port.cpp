// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

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
#include "husarnet/ports/port.h"
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
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "nvs_handle.hpp"

#define NVS_HOSTNAME_KEY "hostname"
#define NVS_IDENTITY_KEY "id"
#define NVS_CONFIG_KEY "config"
#define NVS_LICENSE_KEY "license"
#define NVS_API_SECRET_KEY "api_secret"

static const char* LOG_TAG = "husarnet";
std::unique_ptr<nvs::NVSHandle> nvsHandle;

static void taskProc(void* p)
{
  std::function<void()>* func1 = (std::function<void()>*)p;
  (*func1)();
  delete func1;
  vTaskDelete(NULL);
}

static std::string readNVS(const std::string& path)
{
  size_t len;
  esp_err_t err =
      nvsHandle->get_item_size(nvs::ItemType::SZ, path.c_str(), len);

  if(err == ESP_ERR_NVS_NOT_FOUND) {
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

static bool writeNVS(const std::string& path, const std::string& data)
{
  esp_err_t err = nvsHandle->set_string(path.c_str(), data.c_str());
  if(err != ESP_OK) {
    LOG_ERROR("Unable to update NVS. (Error: %s)", esp_err_to_name(err));
    return false;
  }

  if(nvsHandle->commit() != ESP_OK) {
    LOG_ERROR("Unable to commit NVS. (Error: %s)", esp_err_to_name(err));
    return false;
  }

  return true;
}

namespace Port {
  // This is exposed to user interface via extern in .h file
  SemaphoreHandle_t notifyReadySemaphore;

  void init()
  {
    esp_err_t err;

    nvsHandle = nvs::open_nvs_handle("husarnet", NVS_READWRITE, &err);

    // If NVS is not initialized, try to initialize it
    if(err == ESP_ERR_NVS_NOT_INITIALIZED) {
      LOG_INFO("NVS not initialized. Initializing...");
      err = nvs_flash_init();

      // NVS partition was truncated and needs to be erased
      // Erase and retry init
      if(err == ESP_ERR_NVS_NO_FREE_PAGES ||
         err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        LOG_INFO("Erasing NVS partition...");
        err = nvs_flash_erase();
        if(err != ESP_OK) {
          LOG_CRITICAL(
              "Unable to erase NVS. (Error: %s)", esp_err_to_name(err));
          abort();
        }

        err = nvs_flash_init();
        if(err != ESP_OK) {
          LOG_CRITICAL(
              "Unable to reinitialize NVS. (Error: %s)", esp_err_to_name(err));
          abort();
        }
      }

      else if(err != ESP_OK) {
        LOG_CRITICAL(
            "Unable to initialize NVS. (Error: %s)", esp_err_to_name(err));
        abort();
      }

      nvsHandle = nvs::open_nvs_handle("husarnet", NVS_READWRITE, &err);
    }

    if(err != ESP_OK) {
      LOG_ERROR("Unable to open NVS. (Error: %s)", esp_err_to_name(err));
      abort();
    }

    notifyReadySemaphore = xSemaphoreCreateBinary();

    if(notifyReadySemaphore == NULL) {
      LOG_CRITICAL("Unable to create semaphore");
      abort();
    }
  }

  void threadStart(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    std::function<void()>* func1 = new std::function<void()>;
    *func1 = std::move(func);
    if(xTaskCreate(taskProc, name, stack, func1, priority, NULL) != pdTRUE) {
      LOG_CRITICAL("Unable to create task");
      abort();
    }
  }

  std::map<std::string, std::string> getEnvironmentOverrides()
  {
    return std::map<std::string, std::string>();
  }

  void notifyReady()
  {
    // Send a notification to the user task that the networking stack is ready
    xSemaphoreGive(notifyReadySemaphore);
  }

  void log(const LogLevel level, const std::string& message)
  {
    esp_log_level_t esp_level;

    switch(level) {
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

  int64_t getCurrentTime()
  {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
  }

  const std::string getHumanTime()
  {
    // Human time is currently only used in logging
    // As we use ESP_LOG, we don't need to implement this function
    // TODO long-term: implement this functionality
    return "";
  }

  IpAddress getIpAddressFromInterfaceName(const std::string& interfaceName)
  {
    LOG_ERROR("getIpAddressFromInterfaceName is not implemented");
    return IpAddress();
  }

  extern "C" {
  // Internal wrapper function called from the TCPIP context.
  // See Port::getLocalAddresses().
  static esp_err_t _getLocalAddressesTcpipFunc(void* ctx)
  {
    std::vector<IpAddress>* result = static_cast<std::vector<IpAddress>*>(ctx);
    struct netif* netif;

    for(u8_t idx = 1; (netif = netif_get_by_index(idx)) != NULL; idx++) {
      char buf[IP6ADDR_STRLEN_MAX];
      const ip6_addr_t* ipv6;
      // Iterate over all IPv6 addresses and validate them
      for(int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        ipv6 = netif_ip6_addr(netif, i);
        if(ipv6 == NULL) {
          continue;
        }

        if(ip6_addr_isany(ipv6) || ip6_addr_isloopback(ipv6)) {
          continue;
        }

        // Append to valid addresses
        ip6addr_ntoa_r(ipv6, buf, IP6ADDR_STRLEN_MAX);
        LOG_DEBUG("netif: %s, ip6: %s", netif->name, buf);
        result->push_back(IpAddress::fromBinary(buf));
      }

      // LwIP can have only one IPv4 address per interface
      // Validate address
      const ip4_addr_t* ipv4 = netif_ip4_addr(netif);
      if(ipv4 == NULL) {
        continue;
      }

      if(ip4_addr_isany(ipv4) || ip4_addr_isloopback(ipv4)) {
        continue;
      }

      // Append to valid addresses
      ip4addr_ntoa_r(ipv4, buf, IP4ADDR_STRLEN_MAX);
      LOG_DEBUG("netif: %s, ip4: %s", netif->name, buf);
      result->push_back(IpAddress::fromBinary4(buf));
    }

    return ESP_OK;
  }
  }

  std::vector<IpAddress> getLocalAddresses()
  {
    std::vector<IpAddress> ret;

    // LwIP calls must be performed from the TCPIP context
    esp_err_t res = esp_netif_tcpip_exec(_getLocalAddressesTcpipFunc, &ret);

    if(res != ESP_OK) {
      LOG_ERROR("Failed to get local addresses");
    }

    return ret;
  }

  UpperLayer* startTunTap(HusarnetManager* manager)
  {
    ip6_addr_t ip;
    memcpy(ip.addr, manager->getIdentity()->getDeviceId().data(), 16);

    auto tunTap = new TunTap(ip, 32);
    return tunTap;
  }

  void processSocketEvents(HusarnetManager* manager)
  {
    OsSocket::runOnce(20);  // process socket events for at most so many ms
    manager->getTunTap()->processQueuedPackets();
  }

  std::string getSelfHostname()
  {
    return readNVS(NVS_HOSTNAME_KEY);
  }

  bool setSelfHostname(const std::string& newHostname)
  {
    return writeNVS(NVS_HOSTNAME_KEY, newHostname);
  }

  void updateHostsFile(const std::map<std::string, IpAddress>& data)
  {
    // TODO implement updateHostsFile
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
        auto addr = IpAddress::fromBinary4(
            (uint32_t)((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr);
        freeaddrinfo(result);
        return addr;
      }
      if(res->ai_family == AF_INET6) {
        auto addr = IpAddress::fromBinary(
            (char*)((struct sockaddr_in6*)res->ai_addr)->sin6_addr.s6_addr);
        freeaddrinfo(result);
        return addr;
      }
    }

    freeaddrinfo(result);

    return IpAddress();
  }

  // TODO long-term: implement hooks using FreeRTOS notifications
  bool runScripts(const std::string& path)
  {
    LOG_ERROR("runScripts not supported on ESP32");
    return false;
  }

  bool checkScriptsExist(const std::string& path)
  {
    return false;
  }

  std::string readIdentity()
  {
    return readNVS(NVS_IDENTITY_KEY);
  }

  bool writeIdentity(const std::string& data)
  {
    return writeNVS(NVS_IDENTITY_KEY, data);
  }

  std::string readConfig()
  {
    return readNVS(NVS_CONFIG_KEY);
  }

  bool writeConfig(const std::string& data)
  {
    return writeNVS(NVS_CONFIG_KEY, data);
  }

  std::string readLicenseJson()
  {
    return readNVS(NVS_LICENSE_KEY);
  }

  bool writeLicenseJson(const std::string& data)
  {
    return writeNVS(NVS_LICENSE_KEY, data);
  }

  std::string readApiSecret()
  {
    return readNVS(NVS_API_SECRET_KEY);
  }

  bool writeApiSecret(const std::string& data)
  {
    return writeNVS(NVS_API_SECRET_KEY, data);
  }
}  // namespace Port

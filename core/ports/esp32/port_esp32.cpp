// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <tcpip_adapter.h>

#include "husarnet/ports/port.h"

#include "husarnet/util.h"

#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

bool husarnetVerbose = true;

int64_t currentTime()
{
  // return (xTaskGetTickCount() * 1000 / configTICK_RATE_HZ) + 10000000ul;
  return esp_timer_get_time() / 1000 + 10000000ul;
}

std::vector<IpAddress> getLocalAddresses()
{
  std::vector<IpAddress> ret;
  for(tcpip_adapter_if_t ifid : {TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_IF_AP}) {
    tcpip_adapter_ip_info_t info;
    if(tcpip_adapter_get_ip_info(ifid, &info) == ESP_OK) {
      IpAddress addr = IpAddress::fromBinary4(info.ip.addr);
      ret.push_back(addr);
    }
  }
  return ret;
}

void taskProc(void* p)
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
  assert(stack > 0);
  TaskHandle_t handle;
  std::function<void()>* func1 = new std::function<void()>;
  *func1 = std::move(func);
  int coreId = strcmp(name, "ngsocket") == 0 ? 1 : 0;
  if(xTaskCreatePinnedToCore(
         taskProc, name, stack, func1, priority, &handle, coreId) != pdTRUE) {
    abort();
  }
}

static nvs_handle nvsHandle;
static bool nvsInit;

std::string readBlob(const char* name)
{
  // read key `name` from ESP32 NVS storage

  if(!nvsInit) {
    ESP_ERROR_CHECK(nvs_open("husarnet", NVS_READWRITE, &nvsHandle));
    nvsInit = true;
  }

  size_t requiredSize = 0;
  int ok = nvs_get_blob(nvsHandle, name, NULL, &requiredSize);
  if(ok != ESP_OK)
    return "";
  std::string s;
  s.resize(requiredSize);
  ok = nvs_get_blob(nvsHandle, name, &s[0], &requiredSize);
  if(ok != ESP_OK)
    return "";
  return s;
}

void writeBlob(const char* name, const std::string& data)
{
  if(!nvsInit) {
    ESP_ERROR_CHECK(nvs_open("husarnet", NVS_READWRITE, &nvsHandle));
    nvsInit = true;
  }

  LOG("write %s (len: %d)", name, (int)data.size());

  if(nvs_set_blob(nvsHandle, name, &data[0], data.size()) != ESP_OK) {
    LOG("failed to update key %s", name);
    return;
  }

  if(nvs_commit(nvsHandle) != ESP_OK) {
    LOG("failed to commit key %s", name);
  }
}

// TODO long term - this is a copy of an old code. Will require some massaging
// in order to work
// __attribute__((weak))
// IpAddress resolveIp(const std::string& hostname) {
//     struct addrinfo* result = nullptr;
//     int error;

//     error = getaddrinfo(hostname.c_str(), port.c_str(), NULL, &result);
//     if (error != 0) {
//         return sockaddr_storage {};
//     }

//     for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
//         if (res->ai_family == AF_INET || res->ai_family == AF_INET6) {
//             sockaddr_storage ss {};
//             ss.ss_family = res->ai_family;
//             assert (sizeof(sockaddr_storage) >= res->ai_addrlen);
//             memcpy(&ss, res->ai_addr, res->ai_addrlen);
//             freeaddrinfo(result);
//             return ss;
//         }
//     }

//     freeaddrinfo(result);

//     return sockaddr_storage {};
// }

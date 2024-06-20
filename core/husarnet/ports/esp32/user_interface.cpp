// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "user_interface.h"

#include "husarnet/husarnet_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "port.h"

// C++ API

void husarnetTask(void* manager)
{
  // Init config and logging only to allow configuration before joining the
  // network
  ((HusarnetManager*)manager)->stage1();

  // Wait for setup to be done
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  // Run Husarnet
  ((HusarnetManager*)manager)->runHusarnet();
}

HusarnetClient::HusarnetClient()
{
  husarnetManager = new HusarnetManager();

  // Start HusarnetManager in a separate task
  void* manager = static_cast<void*>(husarnetManager);

  BaseType_t res = xTaskCreate(
      [](void* manager) { husarnetTask(manager); }, "husarnet_task", 16384,
      manager, 7, &husarnetTaskHandle);

  if(res != pdPASS) {
    LOG_ERROR("Failed to create Husarnet task");
    abort();
  }
}

HusarnetClient::~HusarnetClient()
{
  delete husarnetManager;
}

void HusarnetClient::join(const char* hostname, const char* joinCode)
{
  if(started) {
    LOG_ERROR("Cannot join the network twice");
    return;
  }

  started = true;

  // Notify HusarnetManager task that setup stage is done,
  // start the networking stack
  xTaskNotifyGive(husarnetTaskHandle);

  // Wait until networking stack is ready
  xSemaphoreTake(Port::notifyReadySemaphore, portMAX_DELAY);

  // Join the network
  husarnetManager->joinNetwork(joinCode, hostname);
}

void HusarnetClient::setDashboardFqdn(const char* fqdn)
{
  if(started) {
    LOG_ERROR("Cannot set dashboard FQDN after joining the network");
    return;
  }

  husarnetManager->getConfigStorage().setUserSetting(
      UserSetting::dashboardFqdn, fqdn);
}

std::vector<HusarnetPeer> HusarnetClient::listPeers()
{
  auto peers = husarnetManager->getConfigStorage().getHostTable();

  std::vector<HusarnetPeer> peerList;
  peerList.reserve(peers.size());

  for(const auto& [hostname, ip] : peers) {
    peerList.push_back(HusarnetPeer{hostname, ip.str()});
  }

  return peerList;
}

bool HusarnetClient::isJoined()
{
  return husarnetManager->isJoined();
}

std::string HusarnetClient::getIpAddress()
{
  return husarnetManager->getSelfAddress().str();
}

HusarnetManager* HusarnetClient::getManager()
{
  return husarnetManager;
}

// C API

HusarnetClient* husarnet_init()
{
  return new HusarnetClient();
}

void husarnet_join(
    HusarnetClient* client,
    const char* hostname,
    const char* joinCode)
{
  client->join(hostname, joinCode);
}

void husarnet_set_dashboard_fqdn(HusarnetClient* client, const char* fqdn)
{
  client->setDashboardFqdn(fqdn);
}

uint8_t husarnet_is_joined(HusarnetClient* client)
{
  return client->isJoined();
}

uint8_t husarnet_get_ip_address(HusarnetClient* client, char* ip, size_t size)
{
  std::string ipAddress = client->getIpAddress();
  if(ipAddress.size() >= size)
    return 0;

  memcpy(ip, ipAddress.c_str(), ipAddress.size());
  ip[ipAddress.size()] = '\0';

  return 1;
}

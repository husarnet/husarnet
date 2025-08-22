// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "user_interface.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/dashboardapi/response.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "port.h"

// C++ API

void husarnetTask(void* manager)
{
  // Init config and logging only to allow configuration before joining the
  // network
  ((HusarnetManager*)manager)->prepareHusarnet();

  // Run Husarnet
  ((HusarnetManager*)manager)->runHusarnet();
}

HusarnetClient::HusarnetClient()
{
  husarnetManager = new HusarnetManager();

  // Start HusarnetManager in a separate task
  void* manager = static_cast<void*>(husarnetManager);

  BaseType_t res = xTaskCreate(
      [](void* manager) { husarnetTask(manager); }, "husarnet_task", 16384, manager, 7, &husarnetTaskHandle);

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
  auto callback =
    [this, hostname, joinCode]() {
      HusarnetManager* husarnetManager = this->husarnetManager;

      if(started) {
        LOG_ERROR("Cannot join the network twice");
        return;
      }

      if (strlen(joinCode) == 0) {
        LOG_ERROR("Join code cannot be empty");
        return;
      }

      started = true;

      // Wait until networking stack is ready
      xSemaphoreTake(Port::notifyReadySemaphore, portMAX_DELAY);

      if (husarnetManager->configManager->getApiAddress().isInvalid()) {
        LOG_ERROR("API address is not set. Cannot join the network.");
        return;
      }

      etl::string_view hostname_view(hostname);
      if (hostname_view.empty())
        hostname_view = etl::string_view(Port::getSelfHostname().data(), Port::getSelfHostname().size());

      // Join the network
      auto response = dashboardapi::postClaim(
          husarnetManager->configManager->getApiAddress(),
          husarnetManager->myIdentity,
          etl::string_view(joinCode),
          etl::string_view(hostname_view));
      
      if(!response.isSuccessful()) {
        LOG_ERROR("Failed to join the network: %s", response.toString().c_str());
        return;
      }

      LOG_INFO("Device claim successful");
  };

  Port::threadStart(
      callback,
      "husarnet_join_task",
      16384,
      7);
}

// TODO: reintroduce
// std::vector<HusarnetPeer> HusarnetClient::listPeers()
// {
//   auto peers = husarnetManager->configManager->getHostTable();

//   std::vector<HusarnetPeer> peerList;
//   peerList.reserve(peers.size());

//   for(const auto& [hostname, ip] : peers) {
//     peerList.push_back(HusarnetPeer{hostname, ip.str()});
//   }

//   return peerList;
// }

bool HusarnetClient::isJoined()
{
  return husarnetManager->configManager->isClaimed();
}

std::string HusarnetClient::getIpAddress()
{
  return husarnetManager->myIdentity->getIpAddress().toString();
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

void husarnet_join(HusarnetClient* client, const char* hostname, const char* joinCode)
{
  client->join(hostname, joinCode);
}

//TODO: reintroduce
// void husarnet_set_dashboard_fqdn(HusarnetClient* client, const char* fqdn)
// {
//   client->setDashboardFqdn(fqdn);
// }

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

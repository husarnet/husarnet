// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus

#include <string>
#include <vector>

using HusarnetPeer = std::pair<std::string, std::string>;

/*
  HusarnetClient implements Husarnet VPN API. It is a wrapper
  around HusarnetManager and provides a simplified API for the user.
*/
class HusarnetClient {
 private:
  HusarnetManager* husarnetManager;
  TaskHandle_t husarnetTaskHandle;
  bool started = false;

 public:
  HusarnetClient();
  HusarnetClient(const HusarnetClient&) = delete;
  ~HusarnetClient();

  // Joins Husarnet network with given hostname and join code.
  // Network communication is available after a successful join.
  void join(const char* hostname, const char* joinCode);

  // Sets the FQDN of the Husarnet Dashboard. Used for the self-hosted setup.
  void setDashboardFqdn(const char* fqdn);

  // Returns a list of peers connected to the device in Husarnet network.
  std::vector<HusarnetPeer> listPeers();

  // Returns true if the device is connected to Husarnet network.
  bool isJoined();

  // Returns the IP address of the device in Husarnet network.
  std::string getIpAddress();

  // Returns the internal HusarnetManager instance for advanced usage.
  HusarnetManager* getManager();
};
#else
typedef struct HusarnetClient HusarnetClient;
#endif

#define HUSARNET_IP_STR_LEN 40

#ifdef __cplusplus
extern "C" {
#endif

HusarnetClient* husarnet_init();
void husarnet_join(HusarnetClient* client, const char* hostname, const char* joinCode);
void husarnet_set_dashboard_fqdn(HusarnetClient* client, const char* fqdn);
uint8_t husarnet_is_joined(HusarnetClient* client);
uint8_t husarnet_get_ip_address(HusarnetClient* client, char* ip, size_t size);

#ifdef __cplusplus
}
#endif

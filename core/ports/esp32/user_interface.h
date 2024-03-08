#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus

#include <string>
#include <vector>

using HusarnetPeer = std::pair<std::string, std::string>;

class HusarnetManager;

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

    void join(const char* hostname, const char* joinCode);
    void setDashboardFqdn(const char* fqdn);
    std::vector<HusarnetPeer> listPeers();
    bool isJoined();
    HusarnetManager* getManager();
};
#else
typedef struct HusarnetClient HusarnetClient;
#endif

#ifdef __cplusplus
extern "C" {
#endif

HusarnetClient* husarnet_init();
void husarnet_join(HusarnetClient* client, const char* hostname, const char* joinCode);
void husarnet_set_dashboard_fqdn(HusarnetClient* client, const char* fqdn);
uint8_t husarnet_is_joined(HusarnetClient* client);

#ifdef __cplusplus
}
#endif

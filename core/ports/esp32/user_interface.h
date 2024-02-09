#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus

#include "husarnet/husarnet_manager.h"

/*
  HusarnetClient implements Husarnet VPN API. It is a wrapper
  around HusarnetManager and provides a simplified API for the user.
*/
class HusarnetClient {
private:
  HusarnetManager* husarnetManager;
  bool started = false;
public:
    HusarnetClient();
    HusarnetClient(const HusarnetManager&) = delete;
    ~HusarnetClient();

    void start();
    void join(const char* hostname, const char* joinCode);
    HusarnetManager* getManager();
};
#else
typedef struct HusarnetClient HusarnetClient;
#endif

#ifdef __cplusplus
extern "C" {
#endif

HusarnetClient* husarnet_init();
void husarnet_start(HusarnetClient* client);
void husarnet_join(HusarnetClient* client, const char* hostname, const char* joinCode);

#ifdef __cplusplus
}
#endif

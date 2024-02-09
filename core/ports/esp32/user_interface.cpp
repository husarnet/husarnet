// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "user_interface.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// C++ API

HusarnetClient::HusarnetClient()
{
  husarnetManager = new HusarnetManager();
}

HusarnetClient::~HusarnetClient()
{
  delete husarnetManager;
}

void HusarnetClient::start()
{
  void* manager = static_cast<void*>(husarnetManager);

  xTaskCreate([](void* manager) {
    ((HusarnetManager*)manager)->runHusarnet();
  }, "husarnet_task", 16384, manager, 7, NULL);
}

void HusarnetClient::join(const char* hostname, const char* joinCode)
{
  husarnetManager->joinNetwork(joinCode, hostname);
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

void husarnet_start(HusarnetClient* client)
{
  client->start();
}

void husarnet_join(HusarnetClient* client, const char* hostname, const char* joinCode)
{
  client->join(hostname, joinCode);
}
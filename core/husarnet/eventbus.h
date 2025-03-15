// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include "husarnet/config_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/websocket.h"

class EventBus {
 private:
  HusarnetAddress myAddress;
  ConfigManager* configManager;
  WebSocket ws;

  // Connection attempts throttling
  const Time CONNECTION_ATTEMPT_INTERVAL = 1000;
  Time _lastConnectionAttempt = 0;

 public:
  EventBus(HusarnetAddress myAddress, ConfigManager* configManager);

  void init();
  void periodic();

  void _handleMessage(WebSocket::Message& message);
  void _handleGetConfig_ll();                                    // Do an HTTP call to API, read the response
  void _handleGetConfig(const HTTPMessage::Result& httpResult);  // Parse JSON and act on it (i.e.
                                                                 // by modifying the config
};

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
  bool connected = false;

  // Connection attempts throttling
  const Time CONNECTION_ATTEMPT_INTERVAL = 1000;
  Time _lastConnectionAttempt = 0;

 public:
  EventBus(HusarnetAddress myAddress, ConfigManager* configManager);

  void init();
  void periodic();
  bool isConnected();

  void _handleMessage(WebSocket::Message& message);
};

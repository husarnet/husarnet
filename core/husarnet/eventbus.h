// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include "husarnet/websocket.h"
#include "husarnet/ports/port.h"

class HusarnetManager;

class EventBus {
 public:
  EventBus(HusarnetManager* manager): manager(manager) {};

  void init();
  void periodic();
  void _handleMessage(WebSocket::Message& message);

 private:
  WebSocket ws;
  HusarnetManager* manager;

  // Connection attempts throttling
  const Time CONNECTION_ATTEMPT_INTERVAL = 1000;
  Time _lastConnectionAttempt = 0;
};

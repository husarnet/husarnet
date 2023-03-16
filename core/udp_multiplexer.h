// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <stdint.h>

#include "husarnet/ipaddress.h"
#include "husarnet/string_view.h"

class HusarnetManager;
class NgSocketManager;

class UdpMultiplexer {
 protected:
  HusarnetManager* manager;
  NgSocketManager* ngsocket;

  uint16_t runningPort;

  void onPacket(InetAddress address, string_view data);

 public:
  UdpMultiplexer(HusarnetManager* manager, NgSocketManager* ngsocket);

  void start();

  uint16_t getRunningPort();
};

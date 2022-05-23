// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ipaddress.h"
#include "husarnet/ngsocket.h"

const int TEARDOWN_TIMEOUT = 120 * 1000;

struct Peer {
  DeviceId id;
  Time lastPacket = 0;
  Time lastReestablish = 0;

  int latency = -1;  // in ms
  bool connected = false;
  bool reestablishing = false;
  int failedEstablishments = 0;
  InetAddress targetAddress;
  std::string helloCookie;

  std::vector<InetAddress> targetAddresses;
  InetAddress linkLocalAddress;
  std::unordered_set<InetAddress, iphash> sourceAddresses;
  std::vector<std::string> packetQueue;

  bool active()
  {
    return Port::getCurrentTime() - lastPacket < TEARDOWN_TIMEOUT;
  }
};

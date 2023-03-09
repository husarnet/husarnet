// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/base_transports.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"

#include "enum.h"

BETTER_ENUM(BaseConnectionType, int, NONE = 0, TCP = 1, UDP = 2)

class BaseServer : BidirectionalLayer {
 private:
  IpAddress address;  // Inet addresses will be in respective transports

  BaseTCPTransport tcpConnection;
  BaseUDPTransport udpConnection;

 public:
  BaseConnectionType getCurrentConnectionType();
  IpAddress getAddress();
};

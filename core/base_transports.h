// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include <vector>

#include "husarnet/ports/port_interface.h"

#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer_id.h"
#include "husarnet/string_view.h"

#include "enum.h"

class NgSocketManager;

BETTER_ENUM(
    BaseToPeerMessageKind,
    uint8_t,
    HELLO,
    DEVICE_ADDRESSES,
    DATA,
    NAT_OK,
    STATE,
    REDIRECT,
    INVALID)

struct BaseToPeerMessage {
  BaseToPeerMessageKind kind;

  // Hello message
  fstring<16> cookie;

  // State message
  std::vector<InetAddress> udpAddress;
  std::pair<int, int> natTransientRange;

  // Device addresses message
  PeerId peerId;
  std::vector<InetAddress> addresses;

  // Data message
  PeerId source;
  std::string data;

  // Redirect
  InetAddress newBaseAddress;
};

BETTER_ENUM(
    PeerToBaseMessageKind,
    uint8_t,
    REQUEST_INFO,
    DATA,
    INFO,
    NAT_INIT,
    USER_AGENT,
    NAT_OK_CONFIRM,
    NAT_INIT_TRANSIENT,
    INVALID)

struct PeerToBaseMessage {
  PeerToBaseMessageKind kind;

  // All
  fstring<16> cookie;

  // Request info
  PeerId peerId;

  // My info
  std::vector<InetAddress> addresses;

  // Data message
  PeerId target;
  std::string data;

  // NAT init message
  uint64_t counter = 0;

  // User agent
  std::string userAgent;
};

class BaseTransport : public LowerLayer {};

class BaseTCPTransport : public BaseTransport {
 protected:
  NgSocketManager* ngsocket;

  InetAddress addresss;

  Time lastReceivedMessageTime;

 public:
  BaseTCPTransport(NgSocketManager* ngsocket);

  virtual void onUpperLayerData(PeerId peerId, string_view data);
};

class BaseUDPTransport : public BaseTransport {
 protected:
  NgSocketManager* ngsocket;

  InetAddress addresss;

  Time lastReceivedMessageTime;

 public:
  BaseUDPTransport(NgSocketManager* ngsocket);

  virtual void onUpperLayerData(PeerId peerId, string_view data);
};

// TODO - this needs to be moved either to one of the transports or the
// BaseServer class depending on the usage/protocol Transient base addresses. We
// send a packet to one port from a range of base ports (around 20 ports), so we
// get new NAT mapping every time. (20 * 25s = 500s and UDP NAT mappings
// typically last 30s-180s). This exists to get around permanently broken NAT
// mapping (which may occur due to misconfiguration or Linux NAT
// implementation not always assigning the same outbound port)
// int baseTransientPort = 0;
// std::pair<int, int> baseTransientRange = {
//     0, 0};  // ditto (remember that each of the bases can have a different
//     set
//             // here)

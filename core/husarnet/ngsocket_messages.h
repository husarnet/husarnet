// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include <vector>

#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"
#include "husarnet/string_view.h"

#include "enum.h"

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
  HusarnetAddress deviceId;
  std::vector<InetAddress> addresses;

  // Data message
  HusarnetAddress source;
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
  HusarnetAddress deviceId;

  // My info
  std::vector<InetAddress> addresses;

  // Data message
  HusarnetAddress target;
  std::string data;

  // NAT init message
  uint64_t counter = 0;

  // User agent
  std::string userAgent;
};

BETTER_ENUM(PeerToPeerMessageKind, uint8_t, HELLO, HELLO_REPLY, DATA, INVALID)

struct PeerToPeerMessage {
  PeerToPeerMessageKind kind;

  // hello and hello_reply
  HusarnetAddress myId;
  HusarnetAddress yourId;
  std::string helloCookie;

  // data message
  string_view data;
};

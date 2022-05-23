// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ipaddress.h"

struct BaseToPeerMessage {
  enum
  {
    HELLO,
    DEVICE_ADDRESSES,
    DATA,
    NAT_OK,
    STATE,
    REDIRECT,
    INVALID
  } kind;

  // Hello message
  fstring<16> cookie;

  // State message
  std::vector<InetAddress> udpAddress;
  std::pair<int, int> natTransientRange;

  // Device addresses message
  DeviceId deviceId;
  std::vector<InetAddress> addresses;

  // Data message
  DeviceId source;
  std::string data;

  // Redirect
  InetAddress newBaseAddress;
};

struct PeerToBaseMessage {
  enum
  {
    REQUEST_INFO,
    DATA,
    INFO,
    NAT_INIT,
    USER_AGENT,
    NAT_OK_CONFIRM,
    NAT_INIT_TRANSIENT,
    INVALID
  } kind;

  // All
  fstring<16> cookie;

  // Request info
  DeviceId deviceId;

  // My info
  std::vector<InetAddress> addresses;

  // Data message
  DeviceId target;
  std::string data;

  // NAT init message
  uint64_t counter = 0;

  // User agent
  std::string userAgent;
};

struct PeerToPeerMessage {
  enum
  {
    HELLO,
    HELLO_REPLY,
    DATA,
    INVALID
  } kind;

  // hello and hello_reply
  DeviceId myId;
  DeviceId yourId;
  std::string helloCookie;

  // data message
  string_view data;
};

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <list>
#include <unordered_set>
#include <vector>

#include "husarnet/ports/port_interface.h"

#include "husarnet/device_id.h"
#include "husarnet/ipaddress.h"
#include "husarnet/peer_flags.h"

const int TEARDOWN_TIMEOUT = 120 * 1000;

class Peer {
 private:
  friend class PeerContainer;
  friend class NgSocket;
  friend class SecurityLayer;
  friend class CompressionLayer;

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

  bool negotiated = false;

  fstring<32> kxPubkey;
  fstring<32> kxPrivkey;

  fstring<32> txKey;
  fstring<32> rxKey;

  Time lastLatencyReceived = 0;
  Time lastLatencySent = 0;
  fstring<8> heartbeatIdent;

  PeerFlags flags;

 public:
  bool isActive();
  bool isReestablishing();
  bool isTunelled();
  bool isSecure();

  DeviceId getDeviceId();
  IpAddress getIpAddress();

  std::list<InetAddress> getSourceAddresses();
  std::list<InetAddress> getTargetAddresses();
  InetAddress getUsedTargetAddress();
  InetAddress getLinkLocalAddress();
};

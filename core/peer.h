// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <list>
#include <set>
#include <unordered_set>
#include <vector>

#include "husarnet/ports/port_interface.h"

#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer_flags.h"

const int TEARDOWN_TIMEOUT = 120 * 1000;

#include "husarnet/ipaddress.h"
#include "husarnet/peer_id.h"
#include "husarnet/peer_transports.h"

#include "enum.h"

BETTER_ENUM(PeerConnectionType, int, NONE = 0, TCP = 1, UDP = 2, TUNELLED = 3)

class Peer : public BidirectionalLayer {
 private:
  friend class PeerContainer;
  friend class NgSocket;
  friend class SecurityLayer;
  friend class CompressionLayer;

  PeerId id;
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

  std::vector<std::string>
      packetQueue;  // TODO this should be moved to ngsocket honestly

  bool negotiated = false;

  fstring<32> kxPubkey;
  fstring<32> kxPrivkey;

  fstring<32> txKey;
  fstring<32> rxKey;

  Time lastLatencyReceived = 0;
  Time lastLatencySent = 0;
  fstring<8> heartbeatIdent;

  PeerFlags flags;

  std::vector<PeerTransport*> transports;
  PeerTransport* currentTransport = nullptr;
  std::set<std::pair<PeerTransportType, InetAddress>> discoveryData;

 public:
  bool isActive();
  bool isReestablishing();
  bool
  isTunelled();     // TODO this should go away as tunelled connections will be
                    // handled by Base Server related classes (and NgSocket)
  bool isSecure();  // TODO this should go away as a concept as connections are
                    // always secure - may not be fully established, but the
                    // actual data won't ever be insecure

  PeerId getPeerId();  // TODO swap to getPeerId
  IpAddress getIpAddress();

  std::list<InetAddress> getSourceAddresses();
  std::list<InetAddress> getTargetAddresses();
  InetAddress getUsedTargetAddress();
  InetAddress getLinkLocalAddress();

  PeerConnectionType getCurrentPeerConnectionType();

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

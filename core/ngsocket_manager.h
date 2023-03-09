// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/base_container.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_transports.h"

// This will replace the old Ngsocket as an entrypoint to all operations
// It's called "manager" now to indicate that it's role is not to know the
// implementation details but to organize the work of other components and
// redirect incoming data in proper directions.
// All peer discovery related data and message passing should at some point go
// through it.
class HusarnetManager;

class NgSocketManager : public BidirectionalLayer {
 private:
  PeerContainer* peerContainer;
  BaseContainer* baseContainer;

 public:
  NgSocketManager(HusarnetManager* manager);

  void registerDiscoveryData(
      PeerId peerId,
      PeerTransportType transportType,
      InetAddress ipAddr);

  BaseConnectionType getCurrentBaseConnectionType();
  BaseServer* getCurrentBase();

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

// NOTE in case there are no known peers try tunelling - base servers should
// always have some knowlege about others if they're operational
// NOTE logic will be somewhat all over the place - in ngsocket there will only
// be a decision whether packet should be tunelled or go p2p, but choosing the
// right base server or right peer IP/transport will be handled by Peer/Base
// Containers and Peer/Base classes for the respective choices
// NOTE remember to record addresses the peer is actually sending the data from,
// as they may be mangled by NATs along the way

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"
#include "husarnet/peer_transports.h"

class HusarnetManager;
class NgSocketManager;

class PeerContainer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;
  NgSocketManager* ngsocket;

  std::unordered_map<PeerId, Peer*> peers;

  Peer* cachedPeer = nullptr;
  PeerId cachedPeerId;

 public:
  PeerContainer(HusarnetManager* manager, NgSocketManager* ngsocket);

  Peer* createPeer(PeerId id);
  Peer* getPeer(PeerId id);
  Peer* getOrCreatePeer(PeerId id);
  std::unordered_map<PeerId, Peer*> getPeers();

  void registerDiscoveryData(
      PeerId peerId,
      PeerTransportType transportType,
      InetAddress ipAddr);

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

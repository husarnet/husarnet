// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"

class HusarnetManager;

class PeerContainer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;

  std::unordered_map<PeerId, Peer*> peers;

  Peer* cachedPeer = nullptr;
  PeerId cachedPeerId;

 public:
  PeerContainer(HusarnetManager* manager);
  Peer* createPeer(PeerId id);
  Peer* getPeer(PeerId id);
  Peer* getOrCreatePeer(PeerId id);
  std::unordered_map<PeerId, Peer*> getPeers();

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

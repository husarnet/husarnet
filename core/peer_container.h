// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/peer.h"

class HusarnetManager;

class PeerContainer {
 private:
  HusarnetManager* manager;

  std::unordered_map<DeviceId, Peer*> peers;

  Peer* cachedPeer = nullptr;
  DeviceId cachedPeerId;

 public:
  PeerContainer(HusarnetManager* manager);
  Peer* createPeer(DeviceId id);
  Peer* getPeer(DeviceId id);
  Peer* getOrCreatePeer(DeviceId id);
  std::unordered_map<DeviceId, Peer*> getPeers();
};

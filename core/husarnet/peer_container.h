// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <unordered_map>

#include "husarnet/config_manager.h"
#include "husarnet/device_id.h"
#include "husarnet/identity.h"
#include "husarnet/peer.h"

class PeerContainer {
 private:
  ConfigManager* configManager;
  Identity* identity;

  std::unordered_map<DeviceId, Peer*> peers;  // TODO change to ETL container

  // TODO figure out whether this caching is still beneficial
  Peer* cachedPeer = nullptr;
  DeviceId cachedPeerId;

 public:
  PeerContainer(ConfigManager* configManager, Identity* identity);

  Peer* createPeer(DeviceId id);
  Peer* getPeer(DeviceId id);
  Peer* getOrCreatePeer(DeviceId id);

  std::unordered_map<DeviceId, Peer*>
  getPeers();  // TODO change to ETL container
};

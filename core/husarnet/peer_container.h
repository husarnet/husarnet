// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <unordered_map>

#include "husarnet/config_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ipaddress.h"
#include "husarnet/peer.h"

class PeerContainer {
 private:
  ConfigManager* configManager;
  Identity* identity;

  std::unordered_map<HusarnetAddress, Peer*> peers;

  // TODO figure out whether this caching is still beneficial
  Peer* cachedPeer = nullptr;
  HusarnetAddress cachedPeerId;

 public:
  PeerContainer(ConfigManager* configManager, Identity* identity);

  Peer* createPeer(HusarnetAddress id);
  Peer* getPeer(HusarnetAddress id);
  Peer* getOrCreatePeer(HusarnetAddress id);

  std::unordered_map<HusarnetAddress, Peer*>
  getPeers();  // TODO change to ETL container // TODO see how it's used and
               // optimize the shape
};

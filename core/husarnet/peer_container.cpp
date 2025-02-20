// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer_container.h"

#include <sodium.h>

#include "husarnet/logging.h"
#include "husarnet/util.h"

PeerContainer::PeerContainer(ConfigManager* configManager, Identity* identity)
    : configManager(configManager), identity(identity)
{
}

Peer* PeerContainer::createPeer(HusarnetAddress id)
{
  if(!this->configManager->isPeerAllowed(id)) {
    LOG_INFO("peer %s is not on the whitelist", id.toString().c_str());
    return nullptr;
  }
  Peer* peer = new Peer;
  // TODO honestly move this to Peer class (either constructor or some static
  // method)
  peer->heartbeatIdent = generateRandomString(8);
  peer->id = id;
  crypto_kx_keypair(peer->kxPubkey.data(), peer->kxPrivkey.data());
  peers[id] = peer;
  return peer;
}

Peer* PeerContainer::getPeer(HusarnetAddress id)
{
  if(cachedPeerId == id)
    return cachedPeer;

  if(!this->configManager->isPeerAllowed(id)) {
    LOG_INFO("peer %s is not on the whitelist", id.toString().c_str());
    return nullptr;
  }

  // Prevent self-connection (i.e. in case of multicast issue)
  if(id == identity->getDeviceId())
    return nullptr;

  auto it = peers.find(id);
  if(it == peers.end())
    return nullptr;

  cachedPeerId = id;
  cachedPeer = it->second;
  return it->second;
}

Peer* PeerContainer::getOrCreatePeer(HusarnetAddress id)
{
  Peer* peer = getPeer(id);
  if(peer == nullptr) {
    peer = createPeer(id);
  }
  return peer;
}

// TODO: ympek: why are we returning a copy and not a reference here?
std::unordered_map<HusarnetAddress, Peer*> PeerContainer::getPeers()
{
  return peers;
}

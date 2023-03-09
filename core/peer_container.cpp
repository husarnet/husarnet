// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer_container.h"

#include <sodium.h>

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

PeerContainer::PeerContainer(HusarnetManager* manager) : manager(manager)
{
}

Peer* PeerContainer::createPeer(PeerId id)
{
  if(!manager->isPeerAddressAllowed(peerIdToIpAddress(id))) {
    LOG_WARNING("peer %s is not on the whitelist", encodeHex(id).c_str());
    return nullptr;
  }
  Peer* peer = new Peer;
  peer->heartbeatIdent = generateRandomString(8);
  peer->id = id;
  crypto_kx_keypair(peer->kxPubkey.data(), peer->kxPrivkey.data());
  peers[id] = peer;
  return peer;
}

Peer* PeerContainer::getPeer(PeerId id)
{
  if(cachedPeerId == id)
    return cachedPeer;

  if(!manager->isPeerAddressAllowed(peerIdToIpAddress(id))) {
    LOG_WARNING("peer %s is not on the whitelist", encodeHex(id).c_str());
    return nullptr;
  }

  if(id == manager->getIdentity()->getPeerId())
    return nullptr;

  auto it = peers.find(id);
  if(it == peers.end())
    return nullptr;

  cachedPeerId = id;
  cachedPeer = it->second;
  return it->second;
}

Peer* PeerContainer::getOrCreatePeer(PeerId id)
{
  Peer* peer = getPeer(id);
  if(peer == nullptr) {
    peer = createPeer(id);
  }
  return peer;
}

std::unordered_map<PeerId, Peer*> PeerContainer::getPeers()
{
  return peers;
}

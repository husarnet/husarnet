// Copyright (c) 2024 Husarnet sp. z o.o.
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

Peer* PeerContainer::createPeer(DeviceId id)
{
  if(!manager->isPeerAddressAllowed(deviceIdToIpAddress(id))) {
    LOG_INFO("peer %s is not on the whitelist", deviceIdToString(id).c_str());
    return nullptr;
  }
  Peer* peer = new Peer;
  peer->heartbeatIdent = generateRandomString(8);
  peer->id = id;
  crypto_kx_keypair(peer->kxPubkey.data(), peer->kxPrivkey.data());
  peers[id] = peer;
  return peer;
}

Peer* PeerContainer::getPeer(DeviceId id)
{
  if(cachedPeerId == id)
    return cachedPeer;

  if(!manager->isPeerAddressAllowed(deviceIdToIpAddress(id))) {
    LOG_INFO("peer %s is not on the whitelist", deviceIdToString(id).c_str());
    return nullptr;
  }

  if(id == manager->getIdentity()->getDeviceId())
    return nullptr;

  auto it = peers.find(id);
  if(it == peers.end())
    return nullptr;

  cachedPeerId = id;
  cachedPeer = it->second;
  return it->second;
}

Peer* PeerContainer::getOrCreatePeer(DeviceId id)
{
  Peer* peer = getPeer(id);
  if(peer == nullptr) {
    peer = createPeer(id);
  }
  return peer;
}

std::unordered_map<DeviceId, Peer*> PeerContainer::getPeers()
{
  return peers;
}

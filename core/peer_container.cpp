// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer_container.h"

#include <sodium.h>

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/ngsocket_manager.h"
#include "husarnet/util.h"

PeerContainer::PeerContainer(
    HusarnetManager* manager,
    NgSocketManager* ngsocket)
    : manager(manager), ngsocket(ngsocket)
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

// void NgSocket::resendInfoRequests()
// {
//   for(Peer* peer : iteratePeers()) {
//     if(peer->isActive())
//       sendInfoRequestToBase(peer->id);
//   }
// }

// void NgSocket::removeSourceAddress(Peer* peer, InetAddress address)
// {
//   peer->sourceAddresses.erase(address);
//   peerSourceAddresses.erase(address);
// }

// void NgSocket::addSourceAddress(Peer* peer, InetAddress source)
// {
//   assert(peer != nullptr);
//   if(peer->sourceAddresses.find(source) == peer->sourceAddresses.end()) {
//     if(peer->sourceAddresses.size() >= MAX_SOURCE_ADDRESSES) {
//       // remove random old address
//       std::vector<InetAddress> addresses(
//           peer->sourceAddresses.begin(), peer->sourceAddresses.end());
//       removeSourceAddress(peer, addresses[rand() % addresses.size()]);
//     }
//     peer->sourceAddresses.insert(source);

//     if(peerSourceAddresses.find(source) != peerSourceAddresses.end()) {
//       Peer* lastPeer = peerSourceAddresses[source];
//       lastPeer->sourceAddresses.erase(source);
//     }
//     peerSourceAddresses[source] = peer;
//   }
// }

// Peer* NgSocket::findPeerBySourceAddress(InetAddress address)
// {
//   auto it = peerSourceAddresses.find(address);
//   if(it == peerSourceAddresses.end())
//     return nullptr;
//   else
//     return it->second;
// }

// std::vector<Peer*> NgSocket::iteratePeers()
// {
//   std::vector<Peer*> peersVec;
//   for(auto& p : peerContainer->getPeers())
//     peersVec.push_back(p.second);
//   return peersVec;
// }

// void NgSocket::udpPacketReceived(InetAddress source, string_view data)
// {
//   LOG_DEBUG("udp received %s", source.str().c_str());
//   if(source == baseUdpAddress) {
//     baseMessageReceivedUdp(parseBaseToPeerMessage(data));
//   } else {
//     if(data[0] == (char)PeerToPeerMessageKind::HELLO ||
//        data[0] == (char)PeerToPeerMessageKind::HELLO_REPLY) {
//       // "slow" messages are handled on the worker thread to reduce latency
//       if(workerQueue.qsize() < WORKER_QUEUE_SIZE) {
//         std::string dataCopy = data;
//         workerQueue.push([this, source, dataCopy]() {
//           peerMessageReceived(source, parsePeerToPeerMessage(dataCopy));
//         });
//       } else {
//         LOG_ERROR("ngsocket worker queue full");
//       }
//     } else {
//       peerMessageReceived(source, parsePeerToPeerMessage(data));
//     }
//   }
// }

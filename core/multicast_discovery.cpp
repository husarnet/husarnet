// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/multicast_discovery.h"

#include <chrono>
#include <thread>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/peer_id.h"

using namespace std::chrono_literals;

// Protocol on the wire is:
// - uint16_t port to knock at
// - peer id (coded as in ipv6)
// It may be extended to hold an extra discovery methods if needed
class DiscoveryMessage {
 protected:
  PeerId peerId;
  uint16_t port;

 public:
  DiscoveryMessage(PeerId peerId, uint16_t port) : peerId(peerId), port(port)
  {
  }

  PeerId getPeerId()
  {
    return peerId;
  }

  uint16_t getPort()
  {
    return port;
  }

  string_view serialize()
  {
    return pack(port) + peerId;
  }

  static DiscoveryMessage deserialize(string_view data)
  {
    if(data.size() < PEERID_LENGTH + 2) {
      return DiscoveryMessage(BadPeerId, 0);
    }

    uint16_t port = unpack<uint16_t>(data.substr(0, 2));
    PeerId peerId = data.substr(2, PEERID_LENGTH).str();

    return DiscoveryMessage(peerId, port);
  }
};

void MulticastDiscoveryServer::serverThread()
{
  while(true) {
    auto message = DiscoveryMessage(
                       manager->getIdentity()->getPeerId(),
                       ngsocket->getUdpMultiplexer().getRunningPort())
                       .serialize();

    OsSocket::udpSendMulticast(
        InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT}, message);
    OsSocket::udpSendMulticast(
        InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT}, message);

    std::this_thread::sleep_for(5000ms);
  }
}

MulticastDiscoveryServer::MulticastDiscoveryServer(
    HusarnetManager* manager,
    NgSocketManager* ngsocket)
    : manager(manager), ngsocket(ngsocket)
{
}

void MulticastDiscoveryServer::start()
{
  if(!manager->getConfigStorage().getUserSettingBool(
         UserSetting::enableMulticastDiscovery))
    return;

  Port::startThread(
      std::bind(&MulticastDiscoveryServer::serverThread, this), "multDisco");

  OsSocket::udpListenMulticast(
      InetAddress{MULTICAST_ADDR_4, MULTICAST_PORT},
      std::bind(
          &MulticastDiscoveryServer::onPacket, this, std::placeholders::_1,
          std::placeholders::_2));

  OsSocket::udpListenMulticast(
      InetAddress{MULTICAST_ADDR_6, MULTICAST_PORT},
      std::bind(
          &MulticastDiscoveryServer::onPacket, this, std::placeholders::_1,
          std::placeholders::_2));
}

void MulticastDiscoveryServer::onPacket(InetAddress address, string_view data)
{
  // we check if this is from local network to avoid some DoS attacks
  if(!(address.ip.isLinkLocal() || address.ip.isPrivateV4())) {
    return;
  }

  auto discoveryMessage = DiscoveryMessage::deserialize(data);

  if(discoveryMessage.getPeerId() == BadPeerId) {
    return;
  }

  if(discoveryMessage.getPeerId() == manager->getIdentity()->getPeerId()) {
    return;
  }

  auto newAddress = address;
  newAddress.port = discoveryMessage.getPort();

  ngsocket->registerDiscoveryData(
      discoveryMessage.getPeerId(), PeerTransportType::P2P, newAddress);
}

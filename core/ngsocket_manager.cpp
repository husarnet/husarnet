// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ngsocket_manager.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/multicast_discovery.h"

NgSocketManager::NgSocketManager(HusarnetManager* manager) : manager(manager)
{
  baseContainer = new BaseContainer(manager, this);
  peerContainer = new PeerContainer(manager, this);
  multicastDiscoveryServer = new MulticastDiscoveryServer(manager, this);
  udpMultiplexer = new UdpMultiplexer(manager, this);
}

void NgSocketManager::start()
{
  udpMultiplexer->start();
  multicastDiscoveryServer->start();
}

UdpMultiplexer& NgSocketManager::getUdpMultiplexer()
{
  return *udpMultiplexer;
}

void NgSocketManager::registerDiscoveryData(
    PeerId peerId,
    PeerTransportType transportType,
    InetAddress ipAddr)
{
}

BaseConnectionType NgSocketManager::getCurrentBaseConnectionType()
{
  return BaseConnectionType::NONE;  // TODO
}

BaseServer* NgSocketManager::getCurrentBase()
{
  return nullptr;  // TODO
}

void NgSocketManager::onUpperLayerData(PeerId source, string_view data)
{
}

void NgSocketManager::onLowerLayerData(PeerId target, string_view packet)
{
}

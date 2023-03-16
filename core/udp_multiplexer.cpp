// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/udp_multiplexer.h"

#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"

UdpMultiplexer::UdpMultiplexer(
    HusarnetManager* manager,
    NgSocketManager* ngsocket)
    : manager(manager), ngsocket(ngsocket)
{
}

void UdpMultiplexer::start()
{
  runningPort = manager->getConfigStorage().getUserSettingInt(
      UserSetting::udpStartingPort);

  for(;; runningPort++) {
    if(OsSocket::udpListenUnicast(
           runningPort, std::bind(
                            &UdpMultiplexer::onPacket, this,
                            std::placeholders::_1, std::placeholders::_2))) {
      break;
    }
  }

  LOG_INFO("UDP listener bound to %d", runningPort);
}

uint16_t UdpMultiplexer::getRunningPort()
{
  return runningPort;
}

void UdpMultiplexer::onPacket(InetAddress address, string_view data)
{
  // TODO
}

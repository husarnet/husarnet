// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer.h"

bool Peer::isActive()
{
  return Port::getCurrentTime() - lastPacket < TEARDOWN_TIMEOUT;
}

bool Peer::isTunelled()
{
  if(!targetAddress) {
    return true;
  }

  if(!connected) {
    return true;
  }

  return false;
}

bool Peer::isReestablishing()
{
  return reestablishing;
}

bool Peer::isSecure()
{
  return negotiated;
}

HusarnetAddress Peer::getIpAddress()
{
  return id;
}

std::string Peer::getIpAddressString()
{
  return getIpAddress().toString();
}

InetAddress Peer::getUsedTargetAddress()
{
  return targetAddress;
}

InetAddress Peer::getLinkLocalAddress()
{
  return linkLocalAddress;
}

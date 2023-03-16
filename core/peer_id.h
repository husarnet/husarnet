// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"

const int PEERID_LENGTH = 16;

// TODO rework this to a custom class
using PeerId = fstring<PEERID_LENGTH>;

static const PeerId BadPeerId =
    PeerId("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");  //__attribute__((unused));

inline IpAddress peerIdToIpAddress(PeerId id)
{
  return IpAddress::fromBinary(id);
}

inline PeerId peerIdFromIpAddress(IpAddress ip)
{
  return ip.toBinary();
}

inline const char* peerIdToCStr(PeerId id)
{
  return reinterpret_cast<char*>(id.data());
}

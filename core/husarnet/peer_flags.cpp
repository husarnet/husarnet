// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/peer_flags.h"

PeerFlags::PeerFlags()
{
  flags = 0;
  setFlag(PeerFlag::supportsFlags);
}

PeerFlags::PeerFlags(uint64_t bin)
{
  flags = bin;
}

PeerFlags::PeerFlags(std::list<PeerFlag> list)
{
  for(auto flag : list) {
    setFlag(flag);
  }
}

void PeerFlags::setFlag(PeerFlag flag)
{
  flags |= flag._value;
}

bool PeerFlags::checkFlag(PeerFlag flag)
{
  return flags && flag._value;
}

uint64_t PeerFlags::asBin()
{
  return flags;
}

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cstdint>
#include <list>

#include "enum.h"

// Those values are actually hardcoded in the protocol. Do *not* change the
// existing ones! Also - those are meant to be binary flags, so use values like
// 1, 2, 4, 8,…
BETTER_ENUM(PeerFlag, int, supportsFlags = 1, compression = 2)

class PeerFlags {
 private:
  uint64_t flags;

 public:
  PeerFlags();
  PeerFlags(uint64_t bin);
  PeerFlags(std::list<PeerFlag> list);

  void setFlag(PeerFlag flag);
  bool checkFlag(PeerFlag flag);
  uint64_t asBin();
};

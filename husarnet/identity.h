// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include "device_id.h"
#include "fstring.h"

struct Identity {
  fstring<32> pubkey;
  DeviceId deviceId;

  virtual fstring<64> sign(const std::string& data) = 0;

  // TODO add methods for serializing and deserializing so ports don't have to
  // do that
};

struct StdIdentity : Identity {
  fstring<64> privkey;
  fstring<64> sign(const std::string& data) override;
};

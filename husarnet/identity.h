// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include "device_id.h"
#include "fstring.h"

class Identity {
 private:
  fstring<32> pubkey;
  fstring<64> privkey;

  DeviceId deviceId;

 public:
  Identity();  // This will create BadDeviceId. Look below for methods that'll
               // get you something actually usable

  fstring<32> getPubkey();
  DeviceId getDeviceId();

  fstring<64> sign(const std::string& data);
  bool isValid();

  // This will make new Identity (and *not* recover the existing one)
  static Identity create();

  // Those are meant to be saved and recovered from file
  std::string serialize();
  static Identity deserialize(std::string);
};

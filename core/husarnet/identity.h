// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"

class Identity {
 private:
  fstring<32> pubkey;
  fstring<64> privkey;

  HusarnetAddress deviceId;

 public:
  Identity();  // This will create an invalid identity - BadDeviceId. Look
               // below for methods that'll get you something actually usable

  bool isValid();

  fstring<32> getPubkey();
  HusarnetAddress getDeviceId();
  IpAddress getIpAddress();

  fstring<64> sign(const std::string& data);  // Sign data with identity

  // This will make new Identity (and *not* recover the existing one)
  static Identity* create();

  // Those are meant to be saved and recovered from file
  std::string serialize();
  static Identity* deserialize(const std::string& data);

  static Identity* init();  // This will recover the identity from the file,
                            // creating and saving if necessary
};

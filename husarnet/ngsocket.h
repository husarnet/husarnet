// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <map>
#include <vector>
#include "enum.h"
#include "husarnet/device_id.h"
#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"
#include "husarnet/licensing.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/ngsocket_options.h"
#include "husarnet/string_view.h"

class HusarnetManager;

BETTER_ENUM(BaseConnectionType, int, NONE = 0, TCP = 1, UDP = 2)

struct NgSocketDelegate {
  virtual void onDataPacket(DeviceId source, string_view data) = 0;
};

struct NgSocket {
  virtual void sendDataPacket(DeviceId target, string_view data) = 0;
  virtual void periodic() = 0;
  NgSocketOptions* options = new NgSocketOptions;
  NgSocketDelegate* delegate = nullptr;

  virtual BaseConnectionType getCurrentBaseConnectionType()
  {
    return BaseConnectionType::NONE;
  };
  virtual InetAddress getCurrentBaseAddress() { return InetAddress{}; };

  virtual std::string peerInfo(DeviceId id) { return ""; }
  virtual std::string info(
      std::map<std::string, std::string> hosts =
          std::map<std::string, std::string>())
  {
    return "";
  }
  virtual int getLatency(DeviceId peer) { return -1; }

  static NgSocket* create(Identity* id, HusarnetManager* manager);
};

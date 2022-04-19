// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>
#include <map>
#include <vector>
#include "husarnet/device_id.h"
#include "husarnet/fstring.h"
#include "husarnet/ipaddress.h"
#include "husarnet/licensing.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/string_view.h"

class HusarnetManager;

struct NgSocketDelegate {
  virtual void onDataPacket(DeviceId source, string_view data) = 0;
};

struct NgSocketOptions {
  std::function<bool(DeviceId)> isPeerAllowed = [](DeviceId) { return true; };
  std::function<bool(InetAddress)> isAddressAllowed = [](InetAddress) {
    return true;
  };
  std::function<std::string(DeviceId)> peerAddressInfo = [](DeviceId) {
    return "";
  };  // only used in `info()`
  std::function<std::vector<InetAddress>(DeviceId)> additionalPeerIpAddresses =
      [](DeviceId) { return std::vector<InetAddress>{}; };
  InetAddress overrideBaseAddress = InetAddress{};
  int overrideSourcePort = 0;
  bool disableUdp = false;
  bool disableMulticast = false;
  bool useOverrideLocalAddresses = false;
  bool compressionEnabled = false;
  bool disableUdpTunnelling = false;
  bool disableTcpTunnelling = false;
  std::vector<InetAddress> overrideLocalAddresses;
  std::string userAgent;

  void loadFromEnvironment();
};

struct NgSocket {
  virtual void sendDataPacket(DeviceId target, string_view data) = 0;
  virtual void periodic() = 0;
  NgSocketOptions* options = new NgSocketOptions;
  NgSocketDelegate* delegate = nullptr;

  virtual std::string generalInfo(
      std::map<std::string, std::string> hosts =
          std::map<std::string, std::string>())
  {
    return "";
  }
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

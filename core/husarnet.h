// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#pragma once
#include <functional>
#include <map>
#include <vector>
#include "base_config.h"
#include "device_id.h"
#include "fstring.h"
#include "husarnet_crypto.h"
#include "ipaddress.h"
#include "string_view.h"

#ifdef NDEBUG
#error "NDEBUG should not be defined"
#endif

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

  virtual std::string generalInfo(std::map<std::string, std::string> hosts =
                                      std::map<std::string, std::string>()) {
    return "";
  }
  virtual std::string peerInfo(DeviceId id) { return ""; }
  virtual std::string info(std::map<std::string, std::string> hosts =
                               std::map<std::string, std::string>()) {
    return "";
  }
  virtual int getLatency(DeviceId peer) { return -1; }

  static NgSocket* create(Identity* id, BaseConfig* baseConfig);
};

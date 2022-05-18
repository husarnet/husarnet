#pragma once
#include "husarnet/ipaddress.h"

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

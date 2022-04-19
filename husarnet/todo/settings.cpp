// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// #include "settings.h"
// #include <ArduinoJson.h>
// #include <algorithm>
// #include <cstdlib>
// #include <fstream>
// #include <sstream>
// #include <unordered_map>
// #include <unordered_set>
// #include "port.h"
// #include "util.h"

// void Settings::loadInto(NgSocketOptions* options) {
//   for (auto p : settingsFileValues) {
//     LOG("settings value: %s=%s", p.first.c_str(), p.second.c_str());
//   }
//   for (auto p : environValues) {
//     LOG("settings value (set by environment): %s=%s", p.first.c_str(),
//         p.second.c_str());
//   }

//   auto merged = settingsFileValues;
//   for (auto p : environValues)
//     merged[p.first] = p.second;

//   auto getKey = [&](std::string key, std::string defaultValue) -> std::string
//   {
//     if (merged.find(key) != merged.end()) {
//       auto val = merged[key];
//       merged.erase(key);
//       return val;
//     }

//     return defaultValue;
//   };

//   auto getBoolKey = [&](std::string key, bool defaultValue) -> bool {
//     std::string value = getKey(key, "");
//     if (value == "1" || value == "true" || value == "on" || value == "yes")
//       return true;
//     if (value == "0" || value == "false" || value == "off" || value == "no")
//       return false;
//     if (value != "")
//       LOG("invalid value '%s' for option %s", value.c_str(), key.c_str());
//     return defaultValue;
//   };

//   auto getIntKey = [&](std::string key, int defaultValue) -> int {
//     std::string value = getKey(key, "");
//     if (value == "")
//       return defaultValue;
//     auto res = parse_integer(value);
//     if (res.first) {
//       return res.second;
//     } else {
//       LOG("invalid int value '%s' for option %s", value.c_str(),
//       key.c_str()); assert(false);
//     }
//     abort();
//   };

//   auto getInetAddressKey = [&](std::string key) -> InetAddress {
//     std::string value = getKey(key, "");
//     if (value == "")
//       return InetAddress{};

//     InetAddress addr = InetAddress::parse(value);
//     if (!addr) {
//       LOG("invalid IP address value '%s' for option %s", value.c_str(),
//           key.c_str());
//       assert(false);
//     }
//     return addr;
//   };

//   {
//     std::string blacklistStr = getKey("address_blacklist", "");
//     std::vector<IpAddress> blacklist;
//     for (std::string addr : splitWhitespace(blacklistStr))
//       blacklist.push_back(IpAddress::parse(addr));

//     options->isAddressAllowed = [blacklist](InetAddress a) {
//       return std::find(blacklist.begin(), blacklist.end(), a.ip) ==
//              blacklist.end();
//     };
//   }

//   options->compressionEnabled = getBoolKey("enable_compression", false);
//   options->userAgent = getKey("user_agent", options->userAgent);
//   options->overrideBaseAddress = getInetAddressKey("override_base_address");
//   options->overrideSourcePort = getIntKey("source_port", 0);

//   options->disableUdp = getBoolKey("disable_udp", false);
//   options->disableMulticast = getBoolKey("disable_multicast", false);
//   options->disableUdpTunnelling = getBoolKey("disable_udp_tunnelling",
//   false); options->disableTcpTunnelling =
//   getBoolKey("disable_tcp_tunnelling", false);

//   {
//     std::string overrideAddresses = getKey("override_local_addresses", "no");
//     if (overrideAddresses != "no") {
//       std::vector<InetAddress> addresses;
//       for (std::string addr : splitWhitespace(overrideAddresses))
//         addresses.push_back(InetAddress::parse(addr));

//       options->useOverrideLocalAddresses = true;
//       options->overrideLocalAddresses = addresses;
//     }
//   }

//   {
//     std::vector<InetAddress> addresses;
//     for (std::string addr :
//          splitWhitespace(getKey("additional_peer_ip_addresses", "")))
//       addresses.push_back(InetAddress::parse(addr));

//     options->additionalPeerIpAddresses = [addresses](DeviceId id) {
//       // for simplicity, we will return these addresses for all peers
//       return addresses;
//     };
//   }

//   for (auto p : merged) {
//     if (p.first == "verbose" || p.first == "config" || p.first ==
//     "hosts_file")
//       continue;  // handled in other places currently
//     LOG("warning: invalid config variable: %s='%s'", p.first.c_str(),
//         p.second.c_str());
//   }
// }

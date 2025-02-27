// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "husarnet/fstring.h"

class IpAddress {
 public:
  fstring<16> data;

  IpAddress();
  IpAddress(fstring<16>);

  bool operator==(const fstring<16> other) const;
  bool operator==(const IpAddress other) const;
  bool operator<(const IpAddress other) const;
  bool operator!=(const IpAddress other) const;

  bool isValid() const;
  bool isInvalid() const;
  bool isZero() const;
  bool isLinkLocal() const;
  bool isMappedV4() const;
  bool isPrivateV4() const;
  bool isFC94() const;
  bool isWildcard() const;
  bool isLoopback() const;
  bool isMulticast() const;
  bool isPrivateNetworkV4() const;
  bool isReservedNotPrivate() const;

  std::string toBinaryString() const;
  std::string toString() const;

  static IpAddress wildcard();
  static IpAddress parse(const char* s);
  static IpAddress parse(const std::string& s);
  static IpAddress fromBinaryString(std::string s);
  static IpAddress fromBinary(const char* data);
  static IpAddress fromBinary4(uint32_t addr);
  static IpAddress fromBinary4(const char* data);
};

typedef IpAddress HusarnetAddress;
typedef IpAddress InternetAddress;

class InetAddress {
 public:
  IpAddress ip;
  uint16_t port;

  bool operator<(const InetAddress other) const
  {
    return std::make_pair(ip, port) < std::make_pair(other.ip, other.port);
  }

  bool operator==(const InetAddress other) const
  {
    return std::make_pair(ip, port) == std::make_pair(other.ip, other.port);
  }

  bool operator!=(const InetAddress other) const
  {
    return !(*this == other);
  }

  operator bool()
  {
    return ip.isValid();
  }

  // TODO long term - make it not use brackets for IPv4 addresses
  std::string str() const
  {
    if(ip.isMappedV4()) {
      return ip.toString() + ":" + std::to_string(port);
    }

    return "[" + ip.toString() + "]:" + std::to_string(port);
  }

  // TODO long term - make it handle addresses without brackets too (namely IPv4
  // ones)
  static InetAddress parse(std::string s)
  {
    int pos = (int)s.rfind(':');
    if(pos == -1)
      return InetAddress{};
    std::string ipstr = s.substr(0, pos);
    if(ipstr.size() > 2 && ipstr.front() == '[' && ipstr.back() == ']')
      ipstr = ipstr.substr(1, ipstr.size() - 2);
    return InetAddress{
        IpAddress::parse(ipstr), (uint16_t)atoi(s.substr(pos + 1).c_str())};
  }
};

inline size_t hashpair(size_t a, size_t b)
{
  return (a + b + 0x64e9c409) ^ (a << 1);  // no idea if this a good function
}

// specialize hash function for map keys
class iphash {
 public:
  size_t operator()(IpAddress a) const
  {
    size_t v = 0;
    for(int i = 0; i < 4; i++) {
      size_t h = 0;
      memcpy(&h, a.data.data() + (i * 4), 4);
      v = hashpair(v, h);
    }
    return v;
  }

  size_t operator()(InetAddress a) const
  {
    return hashpair(iphash()(a.ip), a.port);
  }
};

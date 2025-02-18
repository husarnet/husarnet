// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "husarnet/fstring.h"

class IpAddress {
 public:
  std::array<unsigned char, 16> data;

 public:
  IpAddress() : data()
  {
  }

  bool operator==(const IpAddress other) const
  {
    return data == other.data;
  }

  bool operator<(const IpAddress other) const
  {
    return data < other.data;
  }

  bool operator!=(const IpAddress other) const
  {
    return !(*this == other);
  }

  operator bool() const
  {
    return *this != IpAddress();
  }

  bool isLinkLocal() const
  {
    if(isMappedV4())
      return data[12] == 169 && data[13] == 254;

    return data[0] == 0xfe && data[1] == 0x80;
  }

  bool isMappedV4() const
  {
    return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\xFF\xFF", 12) == 0;
  }

  bool isPrivateV4() const
  {
    if(!isMappedV4())
      return false;
    if(data[12] == 127)
      return true;
    if(data[12] == 10)
      return true;
    if(data[12] == 192 && data[13] == 168)
      return true;
    if(data[12] == 172 && (data[13] & 0b11110000) == 16)
      return true;
    return false;
  }

  // Husarnet IPv6 address range
  bool isFC94() const
  {
    return data[0] == 0xfc && data[1] == 0x94;
  }

  bool isWildcard() const
  {
    if(isMappedV4())
      return data[12] == 0;
    else
      return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0;
  }

  bool isLoopback() const
  {
    if(isMappedV4())
      return data[12] == 127;
    return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01", 16) == 0;
  }

  bool isMulticast() const
  {
    if(isMappedV4())
      return (data[12] & 0xf0) == 224;
    return data[0] == 0xff;
  }

  bool isPrivateNetworkV4() const
  {
    if(!isMappedV4())
      return false;

    if(data[12] == 10)
      return true;
    if(data[12] == 172 && (data[13] & 0xf0) == 16)
      return true;
    if(data[12] == 192 && data[13] == 168)
      return true;

    return false;
  }

  bool isReservedNotPrivate() const
  {
    if(isPrivateNetworkV4())
      return false;

    if(isWildcard() || isLoopback() || isMulticast() || isLinkLocal() ||
       isFC94())
      return true;

    if(isMappedV4()) {
      if(data[12] == 100 && (data[13] & 0b11000000) == 64)  // CGNAT
        return true;
      if(data[12] == 192 && data[13] == 0 &&
         data[14] == 0)  // IETF Protocol Assignments
        return true;
      if(data[12] == 192 && data[13] == 0 && data[14] == 2)  // TEST-NET-1
        return true;
      if(data[12] == 198 && (data[13] & 0xfe) == 18)  // Benchmarking
        return true;
      if(data[12] == 198 && data[13] == 88 && data[14] == 99)  // 6to4 Relay
        return true;
      if(data[12] == 198 && data[13] == 51 && data[14] == 100)  // TEST-NET-2
        return true;
      if(data[12] == 203 && data[13] == 0 && data[14] == 113)  // TEST-NET-3
        return true;
      if(data[12] == 233 && data[13] == 252 && data[15] == 0)  // MCAST-TEST-NET
        return true;
      if((data[12] & 0xf0) == 240)  // Future use
        return true;
      if(data[12] == 255 && data[13] == 255 && data[14] == 255 &&
         data[15] == 255)  // Broadcast
        return true;
    } else {  // IPv6
      if(memcmp(data.data(), "\x00\x64\xff\x9b\0\0\0\0\0\0\0\0", 12) ==
         0)  // Global translation
        return true;
      if(memcmp(data.data(), "\x00\x64\xff\x9b\x00\x01", 6) ==
         0)  // Private translation
        return true;
      if(memcmp(data.data(), "\x01\x00\0\0\0\0\0\0", 8) == 0)  // Discard prefix
        return true;
      if(memcmp(data.data(), "\x20\x01\x00\x00", 4) == 0)  // Teredo
        return true;
      if(data[0] == 0x20 && data[1] == 0x01 && data[2] == 0 &&
         (data[3] & 0b11110000) == 0x20)  // ORCHIDv2
        return true;
      if(memcmp(data.data(), "\x20\x01\x0d\xb8", 4) == 0)  // Documentation
        return true;
      if(memcmp(data.data(), "\x20\x02", 2) == 0)  // 6to4
        return true;
    }

    return false;
  }

  std::string toBinary() const
  {
    return std::string((char*)data.data(), 16);
  }

  fstring<16> toFstring() const
  {
    return fstring<16>((const char*)data.data());
  }
  std::string toString() const
  {
    return str();
  }

  static IpAddress fromBinary(fstring<16> s)
  {
    IpAddress r;
    memcpy(r.data.data(), s.data(), 16);
    return r;
  }

  static IpAddress fromBinary(std::string s)
  {
    assert(s.size() == 16);
    IpAddress r;
    memcpy(r.data.data(), s.data(), 16);
    return r;
  }

  static IpAddress fromBinary(const char* data)
  {
    IpAddress r;
    memcpy(r.data.data(), data, 16);
    return r;
  }

  static IpAddress fromBinary4(uint32_t addr)
  {
    IpAddress r;
    r.data[10] = 0xFF;
    r.data[11] = 0xFF;
    memcpy(r.data.data() + 12, &addr, 4);
    return r;
  }

  static IpAddress fromBinary4(const char* data)
  {
    IpAddress r;
    r.data[10] = 0xFF;
    r.data[11] = 0xFF;
    memcpy(r.data.data() + 12, data, 4);
    return r;
  }

  std::string str() const;
  std::string ipv4Str() const;
  static IpAddress parse(const char* s);
  static IpAddress parse(const std::string& s)
  {
    return parse(s.c_str());
  }
};

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
    return ip;
  }

  // TODO long term - make it not use brackets for IPv4 addresses
  std::string str() const
  {
    if(ip.isMappedV4()) {
      return ip.str() + ":" + std::to_string(port);
    }

    return "[" + ip.str() + "]:" + std::to_string(port);
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

typedef IpAddress HusarnetAddress;
typedef IpAddress InternetAddress;

inline size_t hashpair(size_t a, size_t b)
{
  return (a + b + 0x64e9c409) ^ (a << 1);  // no idea if this a good function
}

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

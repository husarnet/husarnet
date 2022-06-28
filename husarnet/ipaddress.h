// Copyright (c) 2022 Husarnet sp. z o.o.
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

#if defined(ESP_PLATFORM)
namespace std {
  std::string to_string(int a);
  int stoi(std::string s);
}  // namespace std
#endif

struct IpAddress {
  std::array<unsigned char, 16> data;

  IpAddress() : data() {}

  bool operator==(const IpAddress other) const { return data == other.data; }

  bool operator<(const IpAddress other) const { return data < other.data; }

  bool operator!=(const IpAddress other) const { return !(*this == other); }

  operator bool() const { return *this != IpAddress(); }

  bool isLinkLocal() const { return data[0] == 0xfe && data[1] == 0x80; }

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

  bool isFC94() const { return data[0] == 0xfc && data[1] == 0x94; }

  std::string toBinary() { return std::string((char*)data.data(), 16); }

  fstring<16> toFstring() { return fstring<16>((const char*)data.data()); }
  std::string toString() { return str(); }

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

  std::string str() const;
  std::string ipv4Str() const;
  static IpAddress parse(const char* s);
  static IpAddress parse(const std::string& s) { return parse(s.c_str()); }
};

struct InetAddress {
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

  bool operator!=(const InetAddress other) const { return !(*this == other); }

  operator bool() { return ip; }

  // TODO long term - make it not use brackets for IPv4 addresses
  std::string str() const
  {
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

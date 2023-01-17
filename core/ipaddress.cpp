// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ipaddress.h"

#include "husarnet/ports/port.h"

#if defined(ESP_PLATFORM)
namespace std {
  std::string to_string(int a)
  {
    char buf[15];
    sprintf(buf, "%d", a);
    return buf;
  }

  int stoi(const std::string& s)
  {
    int ret = -1;
    sscanf(s.c_str(), "%d", &ret);
    return ret;
  }
}  // namespace std
#endif

int husarnet_ip6addr_aton(const char* cp, uint8_t* addr);
int husarnet_ip4addr_aton(const char* cp, uint8_t* result);

IpAddress IpAddress::parse(const char* s)
{
  assert(s != nullptr);
  // DBG_ASSERT(s[0] >= 0 && s[0] < 128);
  IpAddress r{};

  if(husarnet_ip4addr_aton(s, (uint8_t*)(r.data.data() + 12)) == 1) {
    r.data[10] = 0xFF;
    r.data[11] = 0xFF;
    return r;
  }

  if(husarnet_ip6addr_aton(s, (uint8_t*)r.data.data()) == 1) {
    return r;
  }

  return IpAddress{};
}

std::string IpAddress::str() const
{
  if(isMappedV4())
    return ipv4Str();

  std::string res;

  for(int i = 0; i < 16; i += 2) {
    char buf[20];
    sprintf(buf, "%02x%02x", data.data()[i], data.data()[i + 1]);
    if(i != 0)
      res += ":";
    res += buf;
  }

  return res;
}

std::string IpAddress::ipv4Str() const
{
  if(!isMappedV4())
    return "";

  return std::to_string(data[12]) + "." + std::to_string(data[13]) + "." +
         std::to_string(data[14]) + "." + std::to_string(data[15]);
}

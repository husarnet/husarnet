#pragma once
// A fake DNS server for use when editing /etc/hosts file is not possible.
#include "husarnet.h"
#include "util.h"

struct ResolveResponse {
  enum { OK = 0, SERVFAIL = 2, NXDOMAIN = 3, REFUSED = 5, IGNORE = 100 } code;
  std::string label;
  IpAddress address;
};

using ResolveFunc = std::function<ResolveResponse(std::string)>;

NgSocket* dnsServerWrap(DeviceId deviceId,
                        NgSocket* sock,
                        ResolveFunc resolver);

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ngsocket.h"

struct NetworkDev {
  static NgSocket* wrap(
      DeviceId deviceId,
      NgSocket* sock,
      std::function<std::vector<DeviceId>(DeviceId)> getMulticastDestinations);

  static NgSocket* wrap(DeviceId deviceId, NgSocket* sock)
  {
    return wrap(
        deviceId, sock, [](DeviceId) { return std::vector<DeviceId>(); });
  }
};

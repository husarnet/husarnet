#pragma once
#include "ngsocket.h"

struct NetworkDev {
  static NgSocket* wrap(
      DeviceId deviceId,
      NgSocket* sock,
      std::function<std::vector<DeviceId>(DeviceId)> getMulticastDestinations);

  static NgSocket* wrap(DeviceId deviceId, NgSocket* sock) {
    return wrap(deviceId, sock,
                [](DeviceId) { return std::vector<DeviceId>(); });
  }
};

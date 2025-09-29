// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

class Tun : public UpperLayer {
 private:
  int fd;
  std::string tunBuffer;

  void close();
  bool isRunning();

  // This is called by the OsSocket as a callback
  void onTunData();

 public:
  Tun(std::string name, bool isTap = false);

  void onLowerLayerData(HusarnetAddress source, string_view data) override;
};

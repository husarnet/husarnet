// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

class TunTap : public UpperLayer {
 private:
  int fd;
  std::string tunBuffer;

  void close();
  bool isRunning();

  // This is called by the OsSocket as a callback
  void onTunTapData();

 public:
  TunTap(std::string name, bool isTap = false);

  void onLowerLayerData(DeviceId source, string_view data) override;
};

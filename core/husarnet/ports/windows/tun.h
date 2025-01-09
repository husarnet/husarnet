// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <mutex>
#include <string>

#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

#include "windef.h"

class HusarnetManager;

class TunTap : public UpperLayer {
 private:
  HANDLE tap_fd;
  std::string tunBuffer;
  std::string selfMacAddr;
  std::mutex threadMutex;

  void close();
  bool isRunning();

  void bringUp();
  std::string getMac();
  string_view read(std::string& buffer);
  void write(string_view data);

  void startReaderThread();
  void onTunTapData();

 public:
  TunTap(HusarnetManager* manager, std::string name, bool isTap = false);

  void onLowerLayerData(DeviceId source, string_view data) override;
};

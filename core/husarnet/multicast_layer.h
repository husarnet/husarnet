// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/config_manager.h"
#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

class MulticastLayer : public BidirectionalLayer {
 private:
  DeviceId myDeviceId;
  ConfigManager* configManager;

 public:
  MulticastLayer(DeviceId myDeviceId, ConfigManager* configmanager);

  void onUpperLayerData(DeviceId source, string_view data) override;
  void onLowerLayerData(DeviceId target, string_view packet) override;
};

// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/ngsocket.h"
#include "husarnet/string_view.h"

class HusarnetManager;

class MulticastLayer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;
  DeviceId deviceId;

 public:
  MulticastLayer(HusarnetManager* manager);

  void onUpperLayerData(DeviceId source, string_view data) override;
  void onLowerLayerData(DeviceId target, string_view packet) override;
};

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ngsocket.h"

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

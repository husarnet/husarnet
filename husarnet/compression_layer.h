// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/husarnet_manager.h"
#include "husarnet/ngsocket_interfaces.h"

class CompressionLayer : public BidirectionalLayer {
 public:
  CompressionLayer(HusarnetManager* manager) {}

  void onUpperLayerData(DeviceId source, string_view data)
  {
    sendToLowerLayer(source, data);
  }

  void onLowerLayerData(DeviceId source, string_view data)
  {
    sendToUpperLayer(source, data);
  };
};

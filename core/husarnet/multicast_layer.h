// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/config_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/string_view.h"

class MulticastLayer : public BidirectionalLayer {
 private:
  HusarnetAddress myDeviceId;
  ConfigManager* configManager;

 public:
  MulticastLayer(HusarnetAddress myDeviceId, ConfigManager* configmanager);

  void onUpperLayerData(HusarnetAddress source, string_view data) override;
  void onLowerLayerData(HusarnetAddress target, string_view packet) override;
};

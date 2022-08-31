// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/device_id.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/string_view.h"

class ConfigStorage;
class HusarnetManager;
class PeerContainer;

class CompressionLayer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;
  ConfigStorage& config;
  PeerContainer* peerContainer;

  std::string compressionBuffer;
  std::string cleartextBuffer;

  bool shouldProceed(DeviceId source);

 public:
  CompressionLayer(HusarnetManager* manager);

  void onUpperLayerData(DeviceId peerId, string_view data);
  void onLowerLayerData(DeviceId peerId, string_view data);
};

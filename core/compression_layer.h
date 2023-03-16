// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/husarnet_manager.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"
#include "husarnet/string_view.h"

class ConfigStorage;
class HusarnetManager;
class PeerContainer;
class NgSocketManager;

class CompressionLayer : public BidirectionalLayer {
 private:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
  NgSocketManager* ngsocket;
  HusarnetManager* manager;
#pragma clang diagnostic pop

  ConfigStorage& config;
  PeerContainer* peerContainer;

  std::string compressionBuffer;
  std::string cleartextBuffer;

  bool shouldProceed(PeerId source);

 public:
  CompressionLayer(NgSocketManager* ngsocket, HusarnetManager* manager);

  void onUpperLayerData(PeerId peerId, string_view data);
  void onLowerLayerData(PeerId peerId, string_view data);
};

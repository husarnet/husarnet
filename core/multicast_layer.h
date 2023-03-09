// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"
#include "husarnet/string_view.h"

class HusarnetManager;

class MulticastLayer : public BidirectionalLayer {
 private:
  HusarnetManager* manager;
  PeerId peerId;

 public:
  MulticastLayer(HusarnetManager* manager);

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

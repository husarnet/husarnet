// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/device_id.h"
#include "husarnet/layer_interfaces.h"
#include "husarnet/peer_container.h"
#include "husarnet/peer_flags.h"
#include "husarnet/string_view.h"

class CompressionLayer : public BidirectionalLayer {
 private:
  PeerContainer* peerContainer;
  PeerFlags* myFlags;

  std::string compressionBuffer;
  std::string cleartextBuffer;

  bool shouldProceed(DeviceId source);

 public:
  CompressionLayer(PeerContainer* peerContainer, PeerFlags* myFlags);

  void onUpperLayerData(DeviceId peerId, string_view data);
  void onLowerLayerData(DeviceId peerId, string_view data);
};

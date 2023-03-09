// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/layer_interfaces.h"
#include "husarnet/peer.h"
#include "husarnet/string_view.h"

class TunTap : public UpperLayer {
 public:
  TunTap();

  void onLowerLayerData(PeerId source, string_view data) override;
};

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

#include "husarnet/layer_interfaces.h"
#include "husarnet/peer_id.h"
#include "husarnet/string_view.h"

class TunTap : public UpperLayer {
 private:
  int fd;
  std::string tunBuffer;

  void close();
  bool isRunning();

  // This is called by the OsSocket as a callback
  void onTunTapData();

 public:
  TunTap(std::string name, bool isTap = false);

  void onLowerLayerData(PeerId source, string_view data) override;
};

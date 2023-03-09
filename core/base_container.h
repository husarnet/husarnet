// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <list>

#include "husarnet/base_server.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/layer_interfaces.h"

class BaseContainer : public BidirectionalLayer {
 protected:
  HusarnetManager* manager;

  std::list<BaseServer*> baseServers;
  BaseServer* currentServer;

  void benchmarkingThread();

 public:
  BaseContainer(HusarnetManager* manager);

  InetAddress getCurrentServer();

  void onUpperLayerData(PeerId source, string_view data) override;
  void onLowerLayerData(PeerId target, string_view packet) override;
};

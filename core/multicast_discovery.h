// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ngsocket_manager.h"

class MulticastDiscoveryServer {
 private:
  NgSocketManager* ngsocket;

  void serverThread();

 public:
  MulticastDiscoveryServer(NgSocketManager* ngsocket);
};

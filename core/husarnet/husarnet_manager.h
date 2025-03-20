// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/config_env.h"
#include "husarnet/config_manager.h"
#include "husarnet/hooks_manager.h"
#include "husarnet/identity.h"
#include "husarnet/ngsocket.h"
#include "husarnet/peer_container.h"
#include "husarnet/security_layer.h"

constexpr int heartbeatPeriodMs = 1000 * 60;  // send heartbeat every minute

class HusarnetManager {
 private:
  Identity* myIdentity = nullptr;

  ConfigEnv* configEnv = nullptr;
  ConfigManager* configManager = nullptr;
  HooksManager* hooksManager = nullptr;

  PeerFlags* myFlags = nullptr;
  PeerContainer* peerContainer = nullptr;

  TunTap* tunTap = nullptr;
  SecurityLayer* securityLayer = nullptr;
  NgSocket* ngsocket = nullptr;

 public:
  HusarnetManager();
  HusarnetManager(const HusarnetManager&) = delete;  // TODO add this to most of the singleton-ish classes in the
                                                     // codebase

  // Constructor initializes the port (so you won't be able to i.e. override the
  // environment variables)
  // prepareHusarnet - initializes most of the amenities - like the config
  // storage so you can alter the config programmatically
  // runHusarnet - actually runs the Husarnet stack (blocking)

  void prepareHusarnet();
  void runHusarnet();
};

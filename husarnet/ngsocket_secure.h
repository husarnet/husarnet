// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet_manager.h"
#include "ngsocket.h"

struct NgSocketSecure {
  static NgSocket* create(Identity* identity, HusarnetManager* manager);
};

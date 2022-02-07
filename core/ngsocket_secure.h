// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#pragma once
#include "husarnet_manager.h"
#include "ngsocket.h"

struct NgSocketSecure {
  static NgSocket* create(Identity* identity, HusarnetManager* manager);
};

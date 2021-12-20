// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#pragma once
#include "base_config.h"
#include "husarnet.h"

struct NgSocketSecure {
  static NgSocket* create(Identity* identity, BaseConfig* baseConfig);
};

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "privileged_interface.h"

namespace Privileged {
void init() {}

void start() {}

std::string readConfig() {
  return "";
}

void writeConfig(std::string) {}
// namespace Privileged

Identity* readIdentity() {
  return NULL;
}

void writeIdentity(Identity* identity) {}
}  // namespace Privileged

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "common.h"
#include <stdlib.h>
#include <string>
#include "filestorage.h"

int getApiPort() {
  return getenv("HUSARNET_API_PORT") ? std::stoi(getenv("HUSARNET_API_PORT"))
                                     : 16216;
}

std::string getApiSecret() {
  // return FileStorage::readHttpSecret();
  // TODO implement this
  return "";
}

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "ngsocket.h"

namespace L2Unwrapper {

NgSocket* wrap(NgSocket* sock, std::string mac = "");

constexpr auto ipAddr = "fc94:8385:160b:88d1:c2ec:af1b:06ac:0001";

}  // namespace L2Unwrapper

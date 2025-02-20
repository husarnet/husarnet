// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

bool isLicenseValid(const json& licenseJson);

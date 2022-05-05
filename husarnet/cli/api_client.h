// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>
#include "httplib.h"

httplib::Client prepareApiClient();

httplib::Result apiPost(
    httplib::Client& client,
    const std::string path,
    httplib::Params& params);
httplib::Result apiPost(httplib::Client& client, const std::string path);

httplib::Result apiGet(httplib::Client& client, const std::string path);


#pragma once
#include <string>
#include "httplib.h"

httplib::Client prepareApiClient();
httplib::Params prepareApiParams();
httplib::Result apiPost(httplib::Client& client,
                        const std::string path,
                        const httplib::Params& params);
httplib::Result apiGet(httplib::Client& client, const std::string path);

#pragma once
#include "filestorage.h"
#include "httplib.h"

bool is_invalid_response(const httplib::Result&);
void handle_invalid_response(const httplib::Result&);
httplib::Params get_http_params_with_secret(const std::string confDir);

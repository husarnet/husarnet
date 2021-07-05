#ifndef CLIENT_HTTP_HANDLING_H
#define CLIENT_HTTP_HANDLING_H

#include "httplib.h"
#include "filestorage.h"

bool is_invalid_response(const httplib::Result &);
void handle_invalid_response(const httplib::Result &);
httplib::Params get_http_params_with_secret(const std::string confDir);

#endif //CLIENT_HTTP_HANDLING_H

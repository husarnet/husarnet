#include "http_handling.h"

bool is_invalid_response(const httplib::Result &result)
{

    if (result.error() != httplib::Error::Success)
    {
        return true;
    }
    return false;
}

void handle_invalid_response(const httplib::Result &result)
{
    if (result.error() != httplib::Error::Success)
    {
        std::cout << "The request failed. Make sure that demon is running" << std::endl;
    }
}

httplib::Params get_http_params_with_secret(const std::string confDir)
{
    httplib::Params params;
    std::string secret = FileStorage::readHttpSecret(confDir);
    params.emplace("secret", secret);
    return params;
}
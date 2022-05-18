// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/cli/api_client.h"
#include "husarnet/util.h"

httplib::Client prepareApiClient()
{
  httplib::Client client((std::string("http://127.0.0.1:16216")).c_str());

  return client;
}

static std::string readFile(std::string path)
{
  std::ifstream f(path);
  if(!f.good()) {
    LOG("failed to open %s", path.c_str());
    exit(1);
  }

  std::stringstream buffer;
  buffer << f.rdbuf();

  return buffer.str();
}
const static std::string apiSecretPath = "/var/lib/husarnet/api_secret";

static void prepareApiParams(httplib::Params& params)
{
  std::string secret = readFile(apiSecretPath);
  params.emplace("secret", secret);
}

void checkInvalidResponse(const httplib::Result& result)
{
  if(result.error() != httplib::Error::Success) {
    throw std::string(
        "The request failed. Make sure that demon is running. (Error kind: ") +
        httplib::to_string(result.error()) + std::string(")");
  }
}

httplib::Result apiPost(
    httplib::Client& client,
    const std::string path,
    httplib::Params& params)
{
  prepareApiParams(params);

  auto result = client.Post(path.c_str(), params);

  checkInvalidResponse(result);

  return result;
}

httplib::Result apiPost(
    httplib::Client& client,
    const std::string path,
    std::map<std::string, std::string> params)
{
  httplib::Params httpParams;

  for(auto it = params.begin(); it != params.end(); it++) {
    httpParams.emplace(it->first, it->second);
  }

  return apiPost(client, path, httpParams);
}

httplib::Result apiPost(httplib::Client& client, const std::string path)
{
  httplib::Params httpParams;
  return apiPost(client, path, httpParams);
}

httplib::Result apiGet(httplib::Client& client, const std::string path)
{
  LOG("making a request to %s", path.c_str());
  auto result = client.Get(path.c_str());

  checkInvalidResponse(result);

  // LOG("got %s", result->body.c_str());

  return result;
}

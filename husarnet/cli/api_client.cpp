
#include "api_client.h"
#include "api_server/common.h"

httplib::Client prepareApiClient() {
  httplib::Client client(
      (std::string("http://127.0.0.1:") + std::to_string(getApiPort()))
          .c_str());

  return client;
}

httplib::Params prepareApiParams() {
  httplib::Params params;
  // TODO link this properly
  // std::string secret =
  // FileStorage::readHttpSecret(FileStorage::getConfigDir());
  // params.emplace("secret", secret);
  return params;
}

void checkInvalidResponse(const httplib::Result& result) {
  if (result.error() != httplib::Error::Success) {
    throw std::string(
        "The request failed. Make sure that demon is running. (Error kind: ") +
        httplib::to_string(result.error()) + std::string(")");
  }
}

httplib::Result apiPost(httplib::Client& client,
                        const std::string path,
                        const httplib::Params& params) {
  auto result = client.Post(path.c_str(), params);

  checkInvalidResponse(result);

  return result;
}

httplib::Result apiGet(httplib::Client& client, const std::string path) {
  auto result = client.Get(path.c_str());

  checkInvalidResponse(result);

  return result;
}

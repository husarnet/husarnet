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
  // @TODO implement this
  return "";
}

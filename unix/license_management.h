// TODO: move license functions from unix/main.cpp here
#ifndef CLIENT_LICENSE_MANAGEMENT_H
#define CLIENT_LICENSE_MANAGEMENT_H

#include "husarnet.h"

std::string retrieveLicense(InetAddress address);

std::string prepareConfigDir();

#endif //CLIENT_LICENSE_MANAGEMENT_H

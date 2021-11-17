// TODO: move license functions from unix/main.cpp here
#ifndef CLIENT_LICENSING_UNIX_H
#define CLIENT_LICENSING_UNIX_H

#include "husarnet.h"

std::string retrieveLicense(InetAddress address);

std::string prepareConfigDir();

#endif //CLIENT_LICENSING_UNIX_H

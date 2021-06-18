#include "license_management.h"
#include "husarnet.h"
#include "self_hosted.h"
#include "util.h"
#include "service.h"

#include <unistd.h>
#include <cstdlib>
#include <fstream>

std::string retrieveLicense(InetAddress address) {
    std::string configDir = prepareConfigDir();
    std::string license = requestLicense(address);

    std::ofstream f (configDir + "license.json");
    if (!f.good()) {
        LOG("failed to write: %s", configDir.c_str());
        exit(1);
    }

    f << license;
    f.close();

    return license;
}

#include "filestorage.h"
#include "service_helper.h"
#include "port.h"
#include "util.h"
#include "husarnet_crypto.h"
#include <fstream>

namespace FileStorage {

std::ifstream openFile(std::string name) {
    std::ifstream f (name);
    if (!f.good()) {
        LOG ("failed to open %s", name.c_str());
    }
    return f;
}

Identity* readIdentity(std::string configDir) {
    std::ifstream f = openFile(configDir + "id");
    if (!f.good()) exit(1);
    std::string ip, pubkeyS, privkeyS;
    f >> ip >> pubkeyS >> privkeyS;
    auto identity = new StdIdentity;
    identity->pubkey = decodeHex(pubkeyS);
    identity->privkey = decodeHex(privkeyS);
    identity->deviceId = IpAddress::parse(ip.c_str()).toBinary();
    return identity;
}

void generateAndWriteId(std::string configDir) {
    LOG("generating ID...");
    auto p = NgSocketCrypto::generateId();

    std::ofstream f (configDir + "id");
    if (!f.good()) {
        LOG("failed to write: %s", configDir.c_str());
        exit(1);
    }

    f << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
      << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
}

}

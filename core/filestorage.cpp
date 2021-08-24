#include "filestorage.h"
#include "service_helper.h"
#include "port.h"
#include "util.h"
#include "husarnet_crypto.h"
#include <fstream>
#include <time.h>
#include <algorithm>
#include <random>
namespace FileStorage {

std::ifstream openFile(std::string name) {
    std::ifstream f (name);
    if (!f.good()) {
        LOG ("failed to open %s", name.c_str());
    }
    return f;
}

Identity* readIdentity(std::string configDir) {
    std::ifstream f = openFile(idFilePath(configDir));
    if (!f.good()) exit(1);
    if (f.peek() == std::ifstream::traits_type::eof()) {
        f.close();
        LOG("generating ID...");
        auto p = NgSocketCrypto::generateId();

        std::ofstream fp (idFilePath(configDir));
        if (!fp.good()) {
            LOG("failed to write: %s", configDir.c_str());
            exit(1);
        }

        fp << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
           << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
        auto identity = new StdIdentity;
        identity->pubkey = p.first;
        identity->privkey = p.second;
        identity->deviceId = IpAddress::parse(IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str().c_str()).toBinary();
        return identity;

    } else {
        std::string ip, pubkeyS, privkeyS;
        f >> ip >> pubkeyS >> privkeyS;
        auto identity = new StdIdentity;
        identity->pubkey = decodeHex(pubkeyS);
        identity->privkey = decodeHex(privkeyS);
        identity->deviceId = IpAddress::parse(ip.c_str()).toBinary();
        return identity;
    }
}

void generateAndWriteId(std::string configDir) {
    LOG("generating ID...");
    auto p = NgSocketCrypto::generateId();

    std::ofstream f (idFilePath(configDir));
    if (!f.good()) {
        LOG("failed to write: %s", configDir.c_str());
        exit(1);
    }

    f << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
      << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
}

void generateAndWriteHttpSecret(std::string configDir)
{
    LOG("Generatting http secret...");
    std::string secret = generateRandomString(64);
    std::ofstream f(httpSecretFilePath(configDir));
    if (!f.good())
    {
        LOG("failed to write: %s", configDir.c_str());
        exit(1);
    }
    f << secret << std::endl;
}

std::string readHttpSecret(std::string configDir)
{
    std::ifstream f = openFile(httpSecretFilePath(configDir));
    if (!f.good())
        exit(1);
    std::string secret;
    f >> secret;
    return secret;
}

std::string generateRandomString(const int length)
{
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> uni('a', 'z');

    std::string result = "";
    for (int i = 0; i < length; i++)
    {
        result += (char)uni(rng);
    }

    return result;
}
}
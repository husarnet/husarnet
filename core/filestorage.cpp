#include "filestorage.h"
#include <time.h>
#include <fstream>
#include "husarnet_crypto.h"
#include "port.h"
#include "service_helper.h"
#include "util.h"

namespace FileStorage {

std::string getConfigDir() {
  return getenv("HUSARNET_CONFIG" ? getenv("HUSARNET_CONFIG")
                                  : "/var/lib/husarnet/");
}

void prepareConfigDir() {
  auto configDir = getConfigDir();

  mkdir(configDir.c_str(), 0700);
  chmod(configDir.c_str(), 0700);
}

std::ifstream openFile(std::string path) {
  std::ifstream f(path);
  if (!f.good()) {
    LOG("failed to open %s", path.c_str());
  }
  return f;
}

std::ifstream writeFile(std::string path, std::string content) {
  std::ofstream f(path);
  if (!f.good()) {
    exit(1);
  }

  f << content;
  f.close();
}

Identity* readIdentity() {
  std::ifstream f = openFile(idFilePath());
  if (!f.good())
    exit(1);
  if (f.peek() == std::ifstream::traits_type::eof()) {
    f.close();
    LOG("generating ID...");
    auto p = NgSocketCrypto::generateId();

    std::ofstream fp(idFilePath());
    if (!fp.good()) {
      LOG("failed to write: %s", idFilePath().c_str());
      exit(1);
    }

    fp << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
       << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
    auto identity = new StdIdentity;
    identity->pubkey = p.first;
    identity->privkey = p.second;
    identity->deviceId =
        IpAddress::parse(
            IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first))
                .str()
                .c_str())
            .toBinary();
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

void generateAndWriteId() {
  LOG("generating ID...");
  auto p = NgSocketCrypto::generateId();

  std::ofstream f(idFilePath());
  if (!f.good()) {
    LOG("failed to write: %s", idFilePath().c_str());
    exit(1);
  }

  f << IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str()
    << " " << encodeHex(p.first) << " " << encodeHex(p.second) << std::endl;
}

void generateAndWriteHttpSecret() {
  LOG("Generatting http secret...");
  std::string secret = generateRandomString(64);
  std::ofstream f(httpSecretFilePath());
  if (!f.good()) {
    LOG("failed to write: %s", httpSecretFilePath().c_str());
    exit(1);
  }
  f << secret << std::endl;
}

std::string readHttpSecret() {
  std::ifstream f = openFile(httpSecretFilePath());
  if (!f.good())
    exit(1);
  std::string secret;
  f >> secret;
  return secret;
}

void saveIp6tablesRuleForDeletion(std::string rule) {
  LOG("Saving Ip6tables rules for later deletion...");
  std::ofstream f(ip6tablesRulesLogPath(), std::ios_base::app);
  if (!f.good()) {
    LOG("failed to write: %s", ip6tablesRulesLogPath().c_str());
    exit(1);
  }
  f << rule << std::endl;
}

}  // namespace FileStorage

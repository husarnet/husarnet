#include "main_common.h"

std::string getWebsetupData() {
    auto respText = sendMsg("get-websetup-token\n");
    auto resp = splitWhitespace(respText);
    if (resp.size() != 2 || resp[0] != "ok") {
        LOG("get-websetup-token returned error (%s)", respText.c_str());
        exit(1);
    }
    std::string token = resp[1];
    std::string webtoken = encodeHex(getMyId()) + token;
    return webtoken;
}

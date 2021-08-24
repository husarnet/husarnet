#include "configmanager.h"
#include "base_config.h"
#include "global_lock.h"
#include "husarnet_config.h"
#include "husarnet_crypto.h"
#include "port.h"
#include "service_helper.h"
#include "sockets.h"
#include "util.h"
#include "filestorage.h"
#include <sstream>

#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <ArduinoJson.h>

ConfigManager::ConfigManager(Identity* identity, BaseConfig* baseConfig, ConfigTable* configTable, HostsFileUpdateFunc hostsFileUpdateFunc, NgSocket* sock, std::string httpSecret, LogManager* logManager = nullptr): identity(identity), baseConfig(baseConfig), sock(sock), hostsFileUpdateFunc(hostsFileUpdateFunc), configTable(configTable), httpSecret(httpSecret), logManager(logManager) {
}

bool ConfigManager::isPeerAllowed(DeviceId id) {
    // TODO: cache
    if (configGet("manual", "whitelist-enabled", "true") == "false")
        return true;

    auto ret = configTable->getValue("whitelist", IpAddress::fromBinary(id).str());
    return ret.size() > 0;
}

std::vector<DeviceId> ConfigManager::getMulticastDestinations(DeviceId id) {
    if (std::string(id).find("\xff\x15\xf2\xd3\xa3\x89") == 0) {
        std::vector<DeviceId> res;

        for (auto row : configTable->listValuesForNetwork("manual", "whitelist")) {
            res.push_back(IpAddress::parse(row.key).toBinary());
        }
        
        return res;
    } else {
        return {};
    }
}

std::string ConfigManager::configGet(std::string networkId, std::string key, std::string defaultValue) {
    auto v = configTable->getValueForNetwork(networkId, "config", key);
    if (v.size() == 0)
        return defaultValue;
    if (v.size() > 1) LOG("warning: multiple values for config key %s", key.c_str());
    return v[0];
}

void ConfigManager::configSet(std::string networkId, std::string key, std::string value) {
    configTable->runInTransaction([&]() {
        configTable->removeValues(networkId, "config", key);
        configTable->insert(ConfigRow{networkId, "config", key, value});
    });
}

std::string ConfigManager::getLegacySharedSecret() {
    std::string secret = configGet("manual", "websetup-secret", "");
    if (secret.size() == 0) {
        configSet("manual", "websetup-secret", encodeHex(randBytes(12)));
        secret = configGet("manual", "websetup-secret", "");
    }

    return secret;
}

void ConfigManager::joinNetwork(std::string joinCode, std::string hostname) {
    auto joinInfo = getJoinInfo(joinCode);

    std::string sharedSecret;

    configTable->runInTransaction([&]() {
        configTable->insert(ConfigRow{"manual", "whitelist", joinInfo.first.str(), "1"});
        configTable->insert(ConfigRow{"manual", "manager", joinInfo.first.str(), "1"});

        sharedSecret = getLegacySharedSecret();
    });

    this->joinCode = joinCode;
    this->joinAsHostname = hostname;

    LOG("sending join request to %s", joinInfo.first.str().c_str());
    sendWebsetupUdp(InetAddress{joinInfo.first, 4800},
        "init-request-join-code\n" + joinInfo.second + "\n" + sharedSecret + "\n" + hostname);
}

IpAddress ConfigManager::resolveHostname(std::string hostname) {
    auto values = configTable->getValue("host", hostname);
    if (values.size() > 0)
        return IpAddress::parse(values[0]);
    else
        return IpAddress{};
}



void ConfigManager::updateHosts() {
    std::vector<std::pair<IpAddress, std::string>> hosts;
    std::unordered_set<std::string> mappedHosts;

    for (std::string netId : configTable->listNetworks()) {
        for (auto row : configTable->listValuesForNetwork(netId, "host")) {
            hosts.push_back({IpAddress::parse(row.value), row.key});
            mappedHosts.insert(row.key);
        }
    }

    {
        // TODO: this should be configurable.
        // Ensure "master" and hostname of device are always mapped.
        if (mappedHosts.find("master") == mappedHosts.end())
            hosts.push_back({IpAddress::parse("::1"), "master"});

        std::string my_hostname = configGet("manual", "hostname", "");

        if (my_hostname.size() > 0)
            if (mappedHosts.find(my_hostname) == mappedHosts.end())
                hosts.push_back({IpAddress::parse("::1"), my_hostname});
    }

    std::sort(hosts.begin(), hosts.end());
    hostsFileUpdateFunc(hosts);
}

std::string ConfigManager::getStatusJson() {
    std::string info = sock->info();
    DynamicJsonDocument doc(info.size() * 2 + 4096);

    doc["info_text"] = info;
    doc["hostname"] = configGet("manual", "hostname", "");

    std::string result;
    serializeJson(doc, result);
    return result;
}

std::string ConfigManager::handleControlPacket(std::string data) {
    long s = data.find("\n");
    if (s == -1) return "fail";

    std::string cmd = data.substr(0, s);
    std::string payload = data.substr(s + 1);

    if (cmd != "status" && cmd != "status-json" && cmd != "whitelist-ls" && cmd != "logs" && cmd != "get-verbosity" && cmd != "logs-size" && cmd != "logs-current")
        LOGV ("control command: %s", cmd.c_str());

    try {
    if (cmd == "whitelist-add") {
        std::string addr = decodeHex(payload);
        if (addr.size() != 16) return "fail";

        configTable->insert(ConfigRow{"manual", "whitelist", IpAddress::fromBinary(addr).str(), "1"});

        return "ok";
    } else if (cmd == "whitelist-rm") {
        std::string addr = decodeHex(payload);
        if (addr.size() != 16) return "fail";
        configTable->remove(ConfigRow{"manual", "whitelist", IpAddress::fromBinary(addr).str(), "1"});

        return "ok";
    } else if (cmd == "whitelist-enable") {
        configSet("manual", "whitelist-enabled", "true");

        return "ok";
    } else if (cmd == "whitelist-disable") {
        configSet("manual", "whitelist-enabled", "false");

        return "ok";
    } else if (cmd == "whitelist-ls") {
         std::vector<ConfigRow> values = configTable->listValuesForNetwork("manual", "whitelist");
         std::string response = "";
        for (auto value : values){
            response += value.key  + "\n";
        }
         return response;

    } else if (cmd == "host-add" || cmd == "host-rm") {
        std::vector<std::string> parts = splitWhitespace(payload);
        if (parts.size() == 2) {
            ConfigRow row {"manual", "host", parts[1], IpAddress::parse(parts[0]).str()};
            if (cmd == "host-add")
                configTable->insert(row);
            else
                configTable->remove(row);
            updateHosts();
            return "ok";
        } else {
            return "error";
        }
    } else if (cmd == "status") {
        return sock->info();
    } else if (cmd == "status-json") {
        return getStatusJson();
    } else if (cmd == "logs") {
        return logManager->getLogs();
    } else if (cmd == "get-verbosity") {
        return std::to_string(logManager->getVerbosity());
    } else if (cmd == "logs-size") {
        return std::to_string(logManager->getSize());
    } else if (cmd == "logs-current") {
        return std::to_string(logManager->getCurrentSize());
    } else if(cmd == "verbosity") {
        std::istringstream reader(payload);
        uint verbosity;
        reader.exceptions(std::istringstream::failbit | std::istringstream::badbit);
        try{
        reader >> verbosity;
        logManager->setVerbosity(verbosity);
        return "ok";
        } catch (std::ios_base::failure &e){
            return "fail";
        }
    } else if(cmd == "logs-resize") {
        std::istringstream reader(payload);
        uint size;
        reader.exceptions(std::istringstream::failbit | std::istringstream::badbit);
        try{
        reader >> size;
        logManager->setSize(size);
        return "ok";
        } catch (std::ios_base::failure &e) {
            return "fail";
        }
    } else if (cmd == "join") {
        int pos = payload.find("\n");
        std::string code = payload.substr(0, pos);
        std::string hostname = payload.substr(pos + 1);

        joinNetwork(code, hostname);
        return "ok";
    } else if (cmd == "reset-received-init-response") {
        initResponseReceived = false;
        return "ok";
    } else if (cmd == "has-received-init-response") {
        return initResponseReceived ? "yes" : "no";
    } else if (cmd == "get-websetup-token") {  // These two commands are used internally to i.e. join a network. There's no need for exposing them to users.
        for (const std::string &websetupHost : baseConfig->getDefaultWebsetupHosts())
            configTable->insert(ConfigRow{"manual", "whitelist", IpAddress::parse(websetupHost).str(), "1"});
        return "ok\n" + getLegacySharedSecret();
    } else if (cmd == "set-websetup-token") {
        for (const std::string &websetupHost : baseConfig->getDefaultWebsetupHosts())
            configTable->insert(ConfigRow{"manual", "whitelist", IpAddress::parse(websetupHost).str(), "1"});
        configSet("manual", "websetup-secret", payload);
        return "ok";

    } else {
        LOG("invalid control command %s", cmd.c_str());
        return "fail";
    }
    } catch(ConfigEditFailed &err) {
        LOG("could not edit configuration: %s", err.what());
        return "fail-config";
    }
}

std::string ConfigManager::handleWebsetupPacket(InetAddress addr, std::string data) {
    std::string sharedSecret = getLegacySharedSecret();

    if (addr.ip && configTable->getValueForNetwork("manual", "manager", addr.str()).size() > 0) {
        LOG ("management packet from invalid host (%s)", addr.str().c_str());
        return "";
    }

    if (data.size() < sharedSecret.size() || !NgSocketCrypto::safeEquals(data.substr(0, sharedSecret.size()), sharedSecret)) {
        LOG ("invalid management shared secret");
        return "";
    }
    data = data.substr(sharedSecret.size());

    long s = data.find("\n");
    if (s <= 5) {
        LOG("bad packet format");
        return "";
    }
    std::string seqnum = data.substr(0, 5);
    std::string cmd = data.substr(5, s - 5);
    std::string payload = data.substr(s + 1);

    if (cmd == "ping")
        LOGV ("remote command: %s", cmd.c_str());
    else
        LOG ("remote command: %s (from %s)", cmd.c_str(), addr.str().c_str());

    try {
    if (cmd == "whitelist-rm" || cmd == "whitelist-add" || cmd == "whitelist-enable" || cmd == "whitelist-disable" || cmd == "host-add" || cmd == "host-rm") {
        std::string res = handleControlPacket(cmd + "\n" + payload);
        return (seqnum + res);
    } else if (cmd == "ping") {
        return (seqnum + "pong");
    } else if (cmd == "set-hostname") {
        std::string hostname = payload;
        LOG("change hostname to '%s'", hostname.c_str());
        configSet("manual", "hostname", hostname);
        ServiceHelper::modifyHostname(hostname);
        return (seqnum + "ok");
    } else if (cmd == "init-response") {
        initResponseReceived = true;
        return (seqnum + "ok");
    } else if (cmd == "get-latency") {
        std::string id = IpAddress::parse(payload).toBinary();
        int latency = sock->getLatency(id);

        return (seqnum + "latency=" + std::to_string(latency));
    } else if (cmd == "cleanup") {
        configTable->runInTransaction([&]() {
            configTable->removeAll("manual", "whitelist");
            configTable->removeAll("manual", "host");
            for (ConfigRow row : configTable->listValuesForNetwork("manual", "manager"))
                configTable->insert(ConfigRow{"manual", "whitelist", row.key, row.value});
        });

        return (seqnum + "ok");
    } else if (cmd == "change-secret") {
        configSet("manual", "websetup-secret", payload);
        return (seqnum + "ok");
    } else if (cmd == "get-version") {
        return (seqnum + ("version=" HUSARNET_VERSION ";ua=") + sock->options->userAgent);
    } else {
        return (seqnum + "bad-command");
    }
    } catch(ConfigEditFailed &err) {
        LOG("could not edit configuration: %s", err.what());
        return (seqnum + "fail-config");
    }
}

void ConfigManager::sendWebsetupUdp(InetAddress address, std::string body) {
    sockaddr_in6 addr {};
    addr.sin6_family = AF_INET6;
    memcpy(&addr.sin6_addr, address.ip.data.data(), 16);
    addr.sin6_port = htons(address.port);
    socklen_t addrsize = sizeof(addr);

    int ret = SOCKFUNC(sendto)(websetupFd, body.data(), body.size(), 0, (sockaddr*)&addr, addrsize);
    if (ret < 0) LOG("websetup UDP send failed (%d)", (int)errno);
}

std::pair<IpAddress, std::string> ConfigManager::getJoinInfo(std::string joinCode) {
    std::string joinCodeHost = baseConfig->getDefaultJoinHost();
    std::string joinCodeOnly = joinCode;

    int slash = joinCodeOnly.find("/");
    if (slash != -1) {
        joinCodeHost = joinCodeOnly.substr(0, slash);
        joinCodeOnly = joinCodeOnly.substr(slash + 1);
    }

    if (joinCodeHost.size() == 21) { // base64 encoding
        std::string decoded = base64Decode("/" + joinCodeHost);
        if (decoded.size() == 16)
            joinCodeHost = IpAddress::fromBinary(decoded).str();
    }

    return std::make_pair(IpAddress::parse(joinCodeHost), joinCodeOnly);
}

void ConfigManager::sendInitRequest() {
    LOG("sending init request...");

    std::string sharedSecret = getLegacySharedSecret();

    for (const std::string &host : baseConfig->getDefaultWebsetupHosts()) {
        std::string req;
        req = "init-request\n";
        sendWebsetupUdp(InetAddress{IpAddress::parse(host), 4800}, req);
        configTable->insert(ConfigRow{"manual", "whitelist", IpAddress::parse(host).str(), "1"});
    }

    if (this->joinCode.size() > 0) {
        auto joinInfo = getJoinInfo(this->joinCode);
        LOG("sending join code request: [%s] [%s] [%s]", joinInfo.first.str().c_str(), joinInfo.second.c_str(), joinCode.c_str());
        configTable->insert(ConfigRow{"manual", "whitelist", joinInfo.first.str(), "1"});
        std::string req = "init-request-join-code\n" + joinInfo.second + "\n" + sharedSecret + "\n" + joinAsHostname;
        sendWebsetupUdp(InetAddress{joinInfo.first, 4800}, req);
    }
}

void ConfigManager::websetupBindSocket() {
    websetupFd = SOCKFUNC(socket)(AF_INET6, SOCK_DGRAM, 0);
    assert(websetupFd > 0);
    sockaddr_in6 addr {};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(5580);

    int ret = SOCKFUNC(bind)(websetupFd, (sockaddr*)&addr, sizeof(addr));
    assert(ret == 0);

    // this timeout is needed, so we can check initResponseReceived

    #ifdef _WIN32
    int timeout = 2000;
    #else
    timeval timeout {}; timeout.tv_sec = 2;
    #endif

    SOCKFUNC(setsockopt)(websetupFd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}

void ConfigManager::websetupThread() {
    int64_t lastSent = 0;
    while (true) {
        if (shouldSendInitRequest) {
            if (!initResponseReceived && currentTime() - lastSent > 1000) {
                lastSent = currentTime();
                // We send 'init request' to all saved servers every few seconds, until we get response from any of them.
                // This is disabled by default in standard Husarnet daemon, because its whitelist and hosts DB is persistent.
                sendInitRequest();
            }
        }

        std::string buffer;
        buffer.resize(1024);
        sockaddr_in6 addr {};
        socklen_t addrsize = sizeof(addr);
        int ret = GIL::unlocked<int>([&]() {
                return SOCKFUNC(recvfrom)(websetupFd, &buffer[0], buffer.size(), 0, (sockaddr*)&addr, &addrsize);
            });

        #ifdef _WIN32
        int err = WSAGetLastError();
        if (ret < 0 && (err == WSAETIMEDOUT || err == WSAECONNRESET)) continue;
        #else
        if (ret < 0 && (errno == EWOULDBLOCK || errno == EINTR)) continue;
        #endif
        assert(ret > 0 && addr.sin6_family == AF_INET6);

        auto ip = IpAddress::fromBinary((char*)&addr.sin6_addr);
        InetAddress address {ip, htons(addr.sin6_port)};

        std::string response = handleWebsetupPacket(address, buffer.substr(0, ret));

        if (response.size() != 0) {
            sendWebsetupUdp(address, response);
        }
    }
}

bool ConfigManager::is_secret_valid(const httplib::Request &req, httplib::Response &res) {
    if (!req.has_param("secret") || req.get_param_value("secret") != httpSecret)
    {
        res.set_content("Invalid control secret", "text/plain");
        return false;
    }

    return true;
}

void ConfigManager::httpThread()
{
    httplib::Server svr;
    svr.Get("/hi", [](const httplib::Request &, httplib::Response &res)
            { res.set_content("Hello World!", "text/plain"); });

    svr.Get("/control/whitelist/ls", [&](const httplib::Request &, httplib::Response &res)
            {
                LOG("remote command: %s", "whitelist-ls");
                std::vector<ConfigRow> values = configTable->listValuesForNetwork("manual", "whitelist");
                std::string response = "";
                for (auto value : values)
                {
                    response += value.key + "\n";
                }
                res.set_content(response.c_str(), "text/plain");
            });

    svr.Get("/control/status", [&](const httplib::Request &req, httplib::Response &res)
            {
                std::map<std::string, std::string> hosts = std::map<std::string, std::string>();
                for (std::string netId : configTable->listNetworks())
                {
                    for (auto row : configTable->listValuesForNetwork(netId, "host"))
                    {
                        std::string address = row.value;
                        std::string hostname = row.key;
                        if (hosts.find(address) == hosts.end())
                        {
                           hosts.insert(std::pair<std::string, std::string> (address, hostname));
                        } else {
                            hosts.find(address)->second += " " + hostname;
                        }
                    }
                }
                res.set_content(sock->info(hosts).c_str(), "text/plain");
            });

    svr.Get("/control/status-json", [&](const httplib::Request &, httplib::Response &res)
            { res.set_content(getStatusJson().c_str(), "text/plain"); });

    svr.Get("/control/logs", [&](const httplib::Request &, httplib::Response &res)
            { res.set_content(logManager->getLogs(), "text/plain"); });

    svr.Get("/control/logs/verbosity", [&](const httplib::Request &, httplib::Response &res)
            { res.set_content(std::to_string(logManager->getVerbosity()), "text/plain"); });

    svr.Get("/control/logs/size", [&](const httplib::Request &, httplib::Response &res)
            { res.set_content(std::to_string(logManager->getSize()), "text/plain"); });

    svr.Get("/control/logs/currentSize", [&](const httplib::Request &, httplib::Response &res)
            { res.set_content(std::to_string(logManager->getCurrentSize()), "text/plain"); });

    svr.Post("/control/join", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: join");
                 if(!is_secret_valid(req, res)) { return; }

                 if (req.has_param("code") && req.has_param("hostname"))
                 {
                     std::string code = req.get_param_value("code");
                     std::string hostname = req.get_param_value("hostname");
                     joinNetwork(code, hostname);
                     res.set_content("ok", "text/plain");
                 }
                 else
                 {
                     res.set_content("Invalid command", "text/plain");
                 }
             });

    svr.Post("/control/reset-received-init-response", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: reset-received-init-response");
                 if(!is_secret_valid(req, res)) { return; }

                 initResponseReceived = false;
                 res.set_content("ok", "text/plain");
             });

    svr.Post("/control/logs/size", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: logs-size");
                 if (!is_secret_valid(req, res))
                 {
                     return;
                 }

                 if (req.has_param("size"))
                 {
                     std::string param = req.get_param_value("size");
                     std::istringstream reader(param);
                     uint size;
                     reader.exceptions(std::istringstream::failbit | std::istringstream::badbit);
                     try
                     {
                         reader >> size;
                         logManager->setSize(size);
                         res.set_content("ok", "text/plain");
                     }
                     catch (std::ios_base::failure &e)
                     {
                         res.set_content("fail", "text/plain");
                     }
                 }
                 else
                 {
                     res.set_content("bad-command", "text/plain");
                 }

                 res.set_content("ok", "text/plain");
             });

     svr.Post("/control/logs/verbosity", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: logs-verbosity");
                 if (!is_secret_valid(req, res))
                 {
                     return;
                 }

                 if (req.has_param("verbosity"))
                 {
                     std::string param = req.get_param_value("verbosity");
                     std::istringstream reader(param);
                     uint verbosity;
                     reader.exceptions(std::istringstream::failbit | std::istringstream::badbit);
                     try
                     {
                         reader >> verbosity;
                         logManager->setVerbosity(verbosity);
                         res.set_content("ok", "text/plain");
                     }
                     catch (std::ios_base::failure &e)
                     {
                         res.set_content("fail", "text/plain");
                     }
                 }
                 else
                 {
                     res.set_content("bad-command", "text/plain");
                 }

                 res.set_content("ok", "text/plain");
             });

    svr.Post("/control/has-received-init-response", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: has-received-init-response");
                 if(!is_secret_valid(req, res)) { return; }

                 initResponseReceived ? res.set_content("yes", "text/plain") : res.set_content("no", "text/plain");
             });

    svr.Post("/control/whitelist/add", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: whitelist-add");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     if (req.has_param("address"))
                     {
                         std::string addr = decodeHex(req.get_param_value("address"));
                         if (addr.size() != 16)
                         {
                             res.set_content("fail", "text/plain");
                         }
                         else
                         {
                             configTable->insert(ConfigRow{"manual", "whitelist", IpAddress::fromBinary(addr).str(), "1"});
                             res.set_content("ok", "text/plain");
                         }
                     }
                     else
                     {
                         res.set_content("fail", "text/plain");
                     }
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.Post("/control/whitelist/rm", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: whitelist-rm");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     if (req.has_param("address"))
                     {
                         std::string addr = decodeHex(req.get_param_value("address"));
                         if (addr.size() != 16)
                         {
                             res.set_content("fail", "text/plain");
                         }
                         else
                         {
                             configTable->remove(ConfigRow{"manual", "whitelist", IpAddress::fromBinary(addr).str(), "1"});
                             res.set_content("ok", "text/plain");
                         }
                     }
                     else
                     {
                         res.set_content("fail", "text/plain");
                     }
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.Post("/control/whitelist/enable", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: whitelist-enable");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     configSet("manual", "whitelist-enabled", "true");
                     res.set_content("ok", "text/plain");
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.Post("/control/whitelist/disable", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: whitelist-disable");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     configSet("manual", "whitelist-enabled", "false");
                     res.set_content("ok", "text/plain");
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.Post("/control/host/add", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: host-add");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     if (req.has_param("hostname") && req.has_param("address"))
                     {
                         std::string addr = req.get_param_value("address");
                         std::string hostname = req.get_param_value("hostname");
                         ConfigRow row{"manual", "host", hostname, IpAddress::parse(addr).str()};
                         configTable->insert(row);
                         updateHosts();
                         res.set_content("ok", "text/plain");
                     }
                     else
                     {
                         res.set_content("bad-command", "text/plain");
                     }
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.Post("/control/host/rm", [&](const httplib::Request &req, httplib::Response &res)
             {
                 LOG("control command: host-rm");
                 if(!is_secret_valid(req, res)) { return; }

                 try
                 {
                     if (req.has_param("hostname") && req.has_param("address"))
                     {
                         std::string addr = req.get_param_value("address");
                         std::string hostname = req.get_param_value("hostname");
                         ConfigRow row{"manual", "host", hostname, IpAddress::parse(addr).str()};
                         configTable->remove(row);
                         updateHosts();
                         res.set_content("ok", "text/plain");
                     }
                     else
                     {
                         res.set_content("bad-command", "text/plain");
                     }
                 }
                 catch (ConfigEditFailed &err)
                 {
                     LOG("could not edit configuration: %s", err.what());
                     res.set_content("fail-config", "text/plain");
                 }
             });

    svr.listen("127.0.0.1", 9999);
}

void ConfigManager::runHusarnet() {
    websetupBindSocket();
    GIL::startThread([this]() { this->websetupThread(); }, "websetup", 6000);
    std::thread t1([this]() { this->httpThread(); });


    while (true) {
        sock->periodic();
        OsSocket::runOnce(1000);
    }
}

std::string ConfigManager::getHostname() {
    return configGet("manual", "hostname", "");
}

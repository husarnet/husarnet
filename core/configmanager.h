#pragma once
#include "base_config.h"
#include "configtable.h"
#include "husarnet.h"
#include "port.h"
#include "threads_port.h"
#include "httplib.h"
#include "logmanager.h"
#include <unordered_set>
#include <vector>

using HostsFileUpdateFunc = std::function<void(std::vector<std::pair<IpAddress, std::string>>)>;

class ConfigManager {
    Identity* identity;
    BaseConfig* baseConfig;
    NgSocket* sock;
    HostsFileUpdateFunc hostsFileUpdateFunc;
    ConfigTable* configTable;
    std::string httpSecret;
    LogManager* logManager;

    std::string joinCode;
    std::string joinAsHostname;
    bool initResponseReceived = false;
    int websetupFd = -1;

    void sendWebsetupUdp(InetAddress address, std::string body);
    void websetupThread();
    void httpThread();
    void websetupBindSocket();
    void sendInitRequest();
    std::string getStatusJson();
    std::pair<IpAddress, std::string> getJoinInfo(std::string joinCode);
    std::string configGet(std::string networkId, std::string key, std::string defaultValue);
    void configSet(std::string networkId, std::string key, std::string value);
    bool is_secret_valid(const httplib::Request &req, httplib::Response &res);
public:
    ConfigManager(Identity* identity, BaseConfig* baseConfig, ConfigTable* configTable, HostsFileUpdateFunc hostsFileUpdateFunc, NgSocket* sock, std::string httpSecret, LogManager* logManager);
    ConfigManager(const ConfigManager&) = delete;

    bool shouldSendInitRequest = false;

    void updateHosts();
    bool isPeerAllowed(DeviceId id);
    void joinNetwork(std::string joinCode, std::string hostname);
    std::string getLegacySharedSecret();
    std::string handleWebsetupPacket(InetAddress addr, std::string data);
    std::string handleControlPacket(std::string data);
    std::vector<DeviceId> getMulticastDestinations(DeviceId id);
    IpAddress resolveHostname(std::string hostname);
    std::string getHostname();

    void runHusarnet();
};

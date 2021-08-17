#include "base_config.h"
#include "configtable.h"
#include "configmanager.h"
#include "memory_configtable.h"
#include "husarnet.h"
#include "husarnet_secure.h"
#include "husarnet_crypto.h"
#include "network_dev.h"
#include "self_hosted.h"
#include "sockets.h"
#include "service_helper.h"
#include "global_lock.h"
#include "husarnet_config.h"

#ifdef ESP_PLATFORM
#define ESP_LWIP 1 // important! or we will get struct layout mismatches
#else
#undef htons
#undef htonl
#undef ntohl
#undef ntohs
#endif
#include <lwip/debug.h>
#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/ip.h>
#include <lwip/mem.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/tcpip.h>
#include <lwip/udp.h>


std::vector<std::pair<IpAddress, std::string>> husarnet_hosts;

namespace ServiceHelper {
bool modifyHostname(std::string newHostname) { return true; }
}

void* ngsocketNetif;

extern "C" void* ngsocketRouteHook(const ip6_addr_t* addr) {
    const char* addrBytes = (const char*)addr;
    if (addrBytes[0] == (char)0xfc && addrBytes[1] == (char)0x94)
        return ngsocketNetif;
    else
        return nullptr;
}
struct LwIpDelegate : public NgSocketDelegate {
    netif* m_netif = new netif;

    NgSocket* socket;
    std::string buffer;
    std::string recvbuffer;
    IpAddress address;

    int packetRecvFd = -1, packetSendFd = -1;
    udp_pcb* packetSendPcb = nullptr;

    LwIpDelegate() {
        buffer.resize(2000);
        recvbuffer.resize(2000);
    }

    void init(DeviceId id) { // main thread
        address = IpAddress::fromBinary(id);

        tcpip_callback([](void* arg) {
            ((LwIpDelegate*)arg)->initOnTcpThread();
        }, this);
    }

    void init2() { // NG thread
#ifdef ESP_PLATFORM
        packetRecvFd = SOCKFUNC(socket)(AF_INET, SOCK_DGRAM, 0);
        sockaddr_in si;
        si.sin_family = AF_INET;
        si.sin_addr.s_addr = htonl(0x7f000001);
        si.sin_port = htons(100);
        assert(SOCKFUNC(bind)(packetRecvFd, (sockaddr*)&si, (socklen_t)sizeof(si)) == 0);
        si.sin_port = htons(101);
        assert(SOCKFUNC(connect)(packetRecvFd, (sockaddr*)&si, (socklen_t)sizeof(si)) == 0);
        int flags = SOCKFUNC(fcntl)(packetRecvFd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        SOCKFUNC(fcntl)(packetRecvFd, F_SETFL, flags);
#else
        int fds[2];
        assert(socketpair(AF_LOCAL, SOCK_DGRAM, 0, fds) == 0);
        packetRecvFd = fds[0];
        packetSendFd = fds[1];
#endif

        OsSocket::bindCustomFd(packetRecvFd, [this]() {
            // TODO: OPT: while loop
            long r = SOCKFUNC(recv)(packetRecvFd, &recvbuffer[0], recvbuffer.size(), 0);
            if (r < 0) return; // spurious wakeup
            // ESP32 takes ~1.5ms to get there
            socket->sendDataPacket(BadDeviceId, string_view(recvbuffer).substr(0, r));
        });
    }

    void runOnce() { // NG thread
        socket->periodic();
        OsSocket::runOnce(1000);
    }

    void initOnTcpThread() { // TCP-IP thread
        LOG ("initOnTcpThread");
        ngsocketNetif = m_netif;
        uint32_t nulladdr = 0;

        auto init_static = [](struct netif *netif) -> err_t {
            auto output_static = [](struct netif *netif, struct pbuf *p, const ip6_addr* addr) ->  err_t{
                auto self = (LwIpDelegate*)netif->state;
                self->output(p);
                return ERR_OK;
            };

            netif->name[0] = 'n';
            netif->name[1] = 'g';
            netif->output_ip6 = output_static;
            return ERR_OK;
        };

        auto ret = netif_add(m_netif,
                  (ip4_addr*)&nulladdr, (ip4_addr*)&nulladdr, (ip4_addr*)&nulladdr,
                  this, init_static, tcpip_input);
        assert (ret == m_netif);

        int8_t idx = 0;
        netif_add_ip6_address(m_netif, (ip6_addr_t*)address.data.data(), &idx);
        netif_ip6_addr_set_state(m_netif, idx, IP6_ADDR_PREFERRED);
        netif_set_up(m_netif);

        #ifdef ESP_PLATFORM
        packetSendPcb = udp_new();
        assert (packetSendPcb != nullptr);
        ip_addr_t localip;
        IP_ADDR4(&localip, 127, 0, 0, 1);
        err_t r = udp_bind(packetSendPcb, (ip_addr_t*)&localip, 101);
        LOG ("bind -> %d", (int)r);
        assert(r == ERR_OK);
        #endif
    }

    void onDataPacket(DeviceId id, string_view data) { // NG thread
        assert (data.size() < 4096);
        pbuf* p = pbuf_alloc(PBUF_LINK, (u16_t)data.size(), PBUF_POOL);
        if (p == nullptr) {
            LOG("packet dropped (pool memory exhausted)");
            return;
        }
        pbuf_take(p, data.data(), (u16_t)data.size());

        m_netif->input(p, (netif*)m_netif);
    }

    void output(struct pbuf* p) { // TCP-IP thread
        // send the packet to NG thread (via loopback UDP or socketpair)
        #ifdef ESP_PLATFORM
        if (packetSendPcb != nullptr) {
            pbuf* p1 = pbuf_alloc(PBUF_TRANSPORT, (u16_t)p->tot_len, PBUF_POOL);
            pbuf_copy(p1, p);

            ip_addr_t localip;
            IP_ADDR4(&localip, 127, 0, 0, 1);
            err_t r = udp_sendto(packetSendPcb, p1, &localip, 100);
            assert(r == ERR_OK);
            pbuf_free(p1);
        } else LOG ("cannot send internal packet");
        #else
        if (packetSendFd != -1) {
            int len = std::min((int)buffer.size(), (int)p->tot_len);
            assert(len < 10000);
            pbuf_copy_partial(p, &buffer[0], (u16_t)len, 0);
            SOCKFUNC(send)(packetSendFd, &buffer[0], len, 0);
        } else LOG ("cannot send internal packet");
        #endif
    }
};

static std::string readConfigData() {
    return readBlob("configdata");
}

static void writeConfigData(const std::string& s) {
    writeBlob("configdata", s);
}

static Identity* readIdentity() {
    std::vector<std::string> idStr = splitWhitespace(readBlob("id"));

    std::string ip = idStr.size() > 0 ? idStr[0] : "";
    std::string pubkeyS = idStr.size() > 1 ? idStr[1] : "";
    std::string privkeyS = idStr.size() > 2 ? idStr[2] : "";

    auto deviceId = IpAddress::parse(ip.c_str()).toBinary();
    auto pubkey = decodeHex(pubkeyS);
    auto privkey = decodeHex(privkeyS);

    if (pubkey.size() != 32 || privkey.size() != 64)
        return nullptr;

    auto identity = new StdIdentity;
    identity->pubkey = pubkey;
    identity->privkey = privkey;
    identity->deviceId = deviceId;
    return identity;
}

void generateAndWriteId() {
    LOG("generating ID...");
    auto p = NgSocketCrypto::generateId();
    std::string serialized =
        IpAddress::fromBinary(NgSocketCrypto::pubkeyToDeviceId(p.first)).str() + " " +
        encodeHex(p.first) + " " + encodeHex(p.second);

    writeBlob("id", serialized);

    LOG("generating ID OK");
}

void updateHostsFile(std::vector<std::pair<IpAddress, std::string>> hosts) {
    // @TODO this only copies stuff to a global variable for use in user's code. LwIP integration still pending
    husarnet_hosts = hosts;
    
    for(auto const& host: hosts) {
        LOG("Updating hosts file for %s", host.second.c_str());
    }
}

static ConfigManager* globalConfigManager = NULL;
static std::string savedJoinCode;
static std::string savedHostname;

extern "C" void husarnet_retrieve_license(const char* hostname);

extern "C" void husarnet_start() {
    std::string license = readBlob("license");
    BaseConfig* baseConfig;
    if (!license.empty()) {
        baseConfig = new BaseConfig(license);
    } else {
        husarnet_retrieve_license(::dashboardHostname);
        license = readBlob("license");
        baseConfig = new BaseConfig(license);
    }

    Identity* identity = readIdentity();
    if (!identity) {
        generateAndWriteId();
        identity = readIdentity();
    }

    LwIpDelegate* d = new LwIpDelegate();
    d->init(identity->deviceId);

    auto threadFunc = [identity, baseConfig, d]() {
        GIL::init();
        d->init2();
        LogManager* logManager = new LogManager(10);
        globalLogManager = logManager;
        NgSocket* sock = NgSocketSecure::create(identity, baseConfig);
        ConfigTable* configTable = createMemoryConfigTable(readConfigData(), writeConfigData);

        ConfigManager configManager (identity, baseConfig, configTable, updateHostsFile, sock, FileStorage::generateRandomString(20), logManager);
        globalConfigManager = &configManager;

        sock->options->isPeerAllowed = [&](DeviceId id) {
            return configManager.isPeerAllowed(id);
        };
        sock->options->userAgent = "ESP32";

        configManager.shouldSendInitRequest = true;
        NgSocket* wrappedSock = sock;
        NgSocket* netDev = NetworkDev::wrap(identity->deviceId, wrappedSock,
            [&](DeviceId id) { return configManager.getMulticastDestinations(id); });

        d->socket = netDev;
        netDev->delegate = d;

        configManager.updateHosts();

        if (savedJoinCode.size() != 0)
            configManager.joinNetwork(savedJoinCode, savedHostname);

        configManager.runHusarnet();
    };
    startThread(threadFunc, "ngsocket", 10000);
}

extern "C" void husarnet_join(const char* joinCode, const char* hostname) {
    GIL::lock();
    if (globalConfigManager) {
        globalConfigManager->joinNetwork(joinCode, hostname);
    } else {
        savedJoinCode = joinCode;
        savedHostname = hostname;
    }
    GIL::unlock();
}

extern "C" void husarnet_retrieve_license(const char* hostname) {
    std::string hostnameStr = hostname;
    if (hostnameStr == "default") {
        hostnameStr = ::dashboardHostname;
    }

    IpAddress ip = OsSocket::resolveToIp(hostnameStr);
    InetAddress address{ip, 80};
    std::string license = requestLicense(address);
    writeBlob("license", license);
}

extern "C" const char* get_logs() {
    if (globalLogManager) {
        return globalLogManager->getLogs().c_str();
    }
    return "";
}

extern "C" const char* husarnet_get_hostname() {
    if (globalConfigManager) {
        return globalConfigManager->getHostname().c_str();
    }

    return "";
}

sockaddr_storage husarnetResolve(const std::string& hostname, const std::string& port) {
    sockaddr_storage storage {};

    IpAddress ip = IpAddress::parse(hostname);

    if (!ip) {
        GIL::lock();
        ip = globalConfigManager->resolveHostname(hostname);
        GIL::unlock();
        if (!ip) return storage;
    }

    sockaddr_in6* addr = (sockaddr_in6*)&storage;

    addr->sin6_family = AF_INET6;
    memcpy(&addr->sin6_addr, ip.data.data(), 16);
    addr->sin6_port = htons(std::stoi(port));

    return storage;
}

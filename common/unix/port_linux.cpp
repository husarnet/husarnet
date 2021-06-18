// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <thread>
#ifndef __ANDROID__
#include <linux/random.h>
#include <syscall.h>
#endif
#include <signal.h>
#include <stdio.h>

#include <ares.h>

#include "port.h"
#include "util.h"
#include "global_lock.h"

bool husarnetVerbose = true;

int64_t currentTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000 / 1000;
}

bool isInterfaceBlacklisted(std::string name) {
    if (name.size() > 2 && name.substr(0, 2) == "zt")
        return true; // sending trafiic over ZT may cause routing loops

    if (name == "hnetl2")
        return true;

    return false;
}

void getIpv4Addresses(std::vector<IpAddress>& ret) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct ifconf configuration {};

    if (fd < 0) return;

    if (ioctl(fd, SIOCGIFCONF, &configuration) < 0) goto error;
    configuration.ifc_buf = (char*)malloc(configuration.ifc_len);
    if (ioctl(fd, SIOCGIFCONF, &configuration) < 0) goto error;

    for (int i=0; i < (int)(configuration.ifc_len / sizeof(ifreq)); i ++) {
        struct ifreq& request = configuration.ifc_req[i];
        std::string ifname = request.ifr_name;
        if (isInterfaceBlacklisted(ifname))
            continue;
        struct sockaddr* addr = &request.ifr_ifru.ifru_addr;
        if (addr->sa_family != AF_INET) continue;

        uint32_t addr32 = ((sockaddr_in*)addr)->sin_addr.s_addr;
        IpAddress address = IpAddress::fromBinary4(addr32);

        if (addr32 == 0x7f000001u) continue;

        ret.push_back(address);
    }
    free(configuration.ifc_buf);
 error:
    close(fd);
    return;
}

static FILE* if_inet6;

void beforeDropCap() {
    if_inet6 = fopen("/proc/self/net/if_inet6", "r");
}

void getIpv6Addresses(std::vector<IpAddress>& ret) {
    bool fileOpenedManually = if_inet6 == nullptr;
    FILE* f = fileOpenedManually ? fopen("/proc/self/net/if_inet6", "r") : if_inet6;
    if (f == nullptr) {
        LOG("failed to open if_inet6 file");
        return;
    }
    fseek(f, 0, SEEK_SET);
    while (true) {
        char buf[100];
        if (fgets(buf, sizeof(buf), f) == nullptr) break;
        std::string line = buf;
        if (line.size() < 32) continue;
        auto ip = IpAddress::fromBinary(decodeHex(line.substr(0, 32)));
        if (ip.isLinkLocal()) continue;
        if (ip == IpAddress::fromBinary("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1")) continue;

        std::string ifname = line.substr(44);
        while (ifname.size() && ifname[0] == ' ') ifname = ifname.substr(1);
        if (isInterfaceBlacklisted(ifname))
            continue;

        ret.push_back(ip);
    }
    if (fileOpenedManually)
        fclose(f);
}

std::vector<IpAddress> getLocalAddresses() {
    std::vector<IpAddress> ret;

    getIpv4Addresses(ret);
    getIpv6Addresses(ret);

    return ret;
}

struct ares_result {
    int status;
    IpAddress address;
};

static void ares_wait(ares_channel channel) {
    int nfds;
    fd_set readers, writers;
    struct timeval tv, *tvp;
    while (1) {
        FD_ZERO(&readers);
        FD_ZERO(&writers);
        nfds = ares_fds(channel, &readers, &writers);
        if (nfds == 0)
            break;
        tvp = ares_timeout(channel, NULL, &tv);
        select(nfds, &readers, &writers, NULL, tvp);
        ares_process(channel, &readers, &writers);
    }
}

static void ares_local_callback(void* arg, int status, int timeouts, struct ares_addrinfo *addrinfo) {
    struct ares_result *result = (struct ares_result*) arg; 
    result->status = status;

    if(status != ARES_SUCCESS) {
        LOG("DNS resolution failed. c-ares status code: %i", status);
        return;
    }

    struct ares_addrinfo_node *node = addrinfo->nodes;
    while(node != NULL) {
        if(node->ai_family == AF_INET) {
            result->address = IpAddress::fromBinary4(reinterpret_cast<sockaddr_in*>(node->ai_addr)->sin_addr.s_addr);
        }
        if(node->ai_family == AF_INET6) {
            result->address = IpAddress::fromBinary((const char*)reinterpret_cast<sockaddr_in6*>(node->ai_addr)->sin6_addr.s6_addr);
        }
        
        node = node->ai_next;
    }

    ares_freeaddrinfo(addrinfo);
}

IpAddress resolveIp(std::string hostname) {
    if(hostname.empty()) {
        return IpAddress();
    }

    struct ares_result result;
    ares_channel channel;

    if(ares_init(&channel) != ARES_SUCCESS) {
        return IpAddress();
    }

    struct ares_addrinfo_hints hints;
    hints.ai_flags |= ARES_AI_NUMERICSERV;

    ares_getaddrinfo(channel, hostname.c_str(),	"443", &hints, ares_local_callback, (void*)&result);
    ares_wait(channel);

    return result.address;
}

void startThread(std::function<void()> func, const char* name, int stack, int priority) {
    std::thread t (func);
    t.detach();
}

bool writeFile(std::string path, std::string data) {
    FILE* f = fopen((path + ".tmp").c_str(), "wb");
    int ret = fwrite(data.data(), data.size(), 1, f);
    if (ret != 1) {
        LOG("could not write to %s (failed to write to temporary file)", path.c_str());
        return false;
    }
    fsync(fileno(f));
    fclose(f);

    if (rename((path + ".tmp").c_str(), path.c_str()) < 0) {
        LOG("could not write to %s (rename failed)", path.c_str());
        return false;
    }
    return true;
}

void initPort() {
#ifndef __ANDROID__
    signal(SIGPIPE, SIG_IGN);
#endif

    GIL::init();
    husarnetVerbose = getenv("HUSARNET_VERBOSE") != nullptr;

    ares_library_init(ARES_LIB_INIT_NONE);
}

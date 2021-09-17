// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#ifndef PORT_H_
#define PORT_H_

#ifdef ESP_PLATFORM
#include "port_esp32.h"
#endif // ESP_PLATFORM

#ifdef _WIN32
#include "windows/port_windows.h"
#endif // _WIN32

#include <string>
#include <cstdint>
#include <vector>
#include <sodium.h>
#include <functional>

#ifdef __linux__
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>

#ifdef USE_HUSARNET_SOCKETS
#include "husarnet_socket.h"
#define SOCKFUNC(name) ::husarnet_##name
#else // USE_HUSARNET_SOCKETS
#define SOCKFUNC(name) ::name
#endif // USE_HUSARNET_SOCKETS

#define ENABLE_IPV6
#endif // __linux__

#include "ipaddress.h"

#ifndef SOCKFUNC_close
#define SOCKFUNC_close SOCKFUNC(close)
inline int renameFile(const char* src, const char* dst) {
    return rename(src, dst);
}
#endif // SOCKFUNC_close

// performance profiler
#define hperf_compute()
#define HPERF_RECORD(name)
// ---



inline int copyFile(const char* src, const char* dst) {
    #ifdef __linux__
    return std::filesystem::copy_file(src, dst, std::filesystem::copy_options::update_existing) ? 0 : -1;
    #else // __linux__
    return -1;  // @TODO - technically we should make it work also on other ports, but in practice I think this will be resolved upper in the call tree
    #endif // __linux__
}

bool writeFile(std::string path, std::string data);

int64_t currentTime();
void startThread(std::function<void()> func, const char* name, int stack=-1, int priority=2);

inline std::string randBytes(int count) {
    std::string s;
    s.resize(count);
    randombytes_buf(&s[0], count);
    return s;
}

void beforeDropCap();
std::vector<IpAddress> getLocalAddresses();

void initPort();

IpAddress resolveIp(std::string hostname);
extern bool husarnetVerbose;
#endif

// port.h has to be the first include, newline afterwards prevents autoformatter
// from sorting it
#include "port.h"

#include <iphlpapi.h>
#include <mswsock.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <chrono>
#include <vector>
#include "global_lock.h"
#include "threads_port.h"
#include "util.h"

bool husarnetVerbose = true;

InetAddress ipFromSockaddr(sockaddr_storage* st) {
  if (st->ss_family == AF_INET) {
    struct sockaddr_in* st4 = (sockaddr_in*)(st);
    InetAddress r{};
    r.ip.data[10] = 0xFF;  // ipv4-mapped ipv6
    r.ip.data[11] = 0xFF;
    memcpy((char*)r.ip.data.data() + 12, &st4->sin_addr, 4);
    r.port = htons(st4->sin_port);
    return r;
  } else if (st->ss_family == AF_INET6) {
    struct sockaddr_in6* st6 = (sockaddr_in6*)(st);
    InetAddress r{};
    memcpy(r.ip.data.data(), &st6->sin6_addr, 16);
    r.port = htons(st6->sin6_port);
    return r;
  } else {
    return InetAddress();
  }
}

int64_t currentTime() {
  using namespace std::chrono;
  milliseconds ms =
      duration_cast<milliseconds>(system_clock::now().time_since_epoch());
  return ms.count();
}

std::vector<IpAddress> getLocalAddresses() {
  // this takes 18 ms on a test system
  std::vector<IpAddress> result;

  int ret;
  PIP_ADAPTER_ADDRESSES buffer;
  unsigned long buffer_size = 20000;

  while (true) {
    buffer = (PIP_ADAPTER_ADDRESSES) new char[buffer_size];

    ret = GetAdaptersAddresses(
        AF_UNSPEC,
        GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_SKIP_FRIENDLY_NAME |
            GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST |
            GAA_FLAG_SKIP_ANYCAST,
        0, buffer, &buffer_size);
    if (ret != ERROR_BUFFER_OVERFLOW)
      break;
    delete[] buffer;
    buffer = nullptr;
    buffer_size *= 2;
  }

  PIP_ADAPTER_ADDRESSES current_adapter = buffer;
  while (current_adapter) {
    PIP_ADAPTER_UNICAST_ADDRESS current = current_adapter->FirstUnicastAddress;
    while (current) {
      InetAddress addr = ipFromSockaddr(
          reinterpret_cast<sockaddr_storage*>(current->Address.lpSockaddr));
      // LOG("detected IP: %s", addr.str().c_str());
      result.push_back(addr.ip);
      current = current->Next;
    }
    current_adapter = current_adapter->Next;
  }

  delete[] buffer;
  buffer = nullptr;
  return result;
}

thread_local const char* threadName = nullptr;

const char* getThreadName() {
  return threadName ? threadName : "null";
}

void runThread(void* arg) {
  auto f = (std::pair<char*, std::function<void()>>*)arg;
  threadName = f->first;
  f->second();
}

void startThread(std::function<void()> func,
                 const char* name,
                 int stack,
                 int priority) {
  auto* f =
      new std::pair<const char*, std::function<void()>>(name, std::move(func));
  _beginthread(runThread, 0, f);
}

void initPort() {
  threadName = "main";
  GIL::init();
  WSADATA wsaData;
  WSAStartup(0x202, &wsaData);
}

IpAddress resolveIp(std::string hostname) {
  struct addrinfo* result = nullptr;
  int error;

  error = SOCKFUNC(getaddrinfo)(hostname.c_str(), "443", NULL, &result);
  if (error != 0) {
    return IpAddress();
  }

  for (struct addrinfo* res = result; res != NULL; res = res->ai_next) {
    if (res->ai_family == AF_INET || res->ai_family == AF_INET6) {
      sockaddr_storage ss{};
      ss.ss_family = res->ai_family;
      assert(sizeof(sockaddr_storage) >= res->ai_addrlen);
      memcpy(&ss, res->ai_addr, res->ai_addrlen);
      SOCKFUNC(freeaddrinfo)(result);
      return ipFromSockaddr(&ss).ip;
    }
  }

  SOCKFUNC(freeaddrinfo)(result);

  return IpAddress();
}

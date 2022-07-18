// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <chrono>
#include <vector>
#include <fstream>


#include "husarnet/ports/port.h"
#include "husarnet/ports/threads_port.h"
#include "husarnet/ports/sockets.h"

#include "shlwapi.h"

#include "husarnet/gil.h"
#include "husarnet/util.h"

bool husarnetVerbose = true;

void runThread(void* arg)
{
  auto f = (std::pair<char*, std::function<void()>>*)arg;
  // threadName = f->first;
  f->second();
}


namespace Port {
  void init() 
  {
    // GIL is inited on (stage1)
    // GIL::init();
    WSADATA wsaData;
    WSAStartup(0x202, &wsaData);
  }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    // old startThread function usss _beginthread which is windows api
    // good.
    auto* f =
        new std::pair<const char*, std::function<void()>>(name, std::move(func));
    _beginthread(::runThread, 0, f);
  }

  IpAddress resolveToIp(std::string hostname)
  {
    // we are using raw getaddrinfo, not ares
    // this might be not cool
    // but also it might work
    struct addrinfo* result = nullptr;
    int error;

    error = SOCKFUNC(getaddrinfo)(hostname.c_str(), "443", NULL, &result);
    if(error != 0) {
      return IpAddress();
    }

    for(struct addrinfo* res = result; res != NULL; res = res->ai_next) {
      if(res->ai_family == AF_INET || res->ai_family == AF_INET6) {
        sockaddr_storage ss{};
        ss.ss_family = res->ai_family;
        assert(sizeof(sockaddr_storage) >= res->ai_addrlen);
        memcpy(&ss, res->ai_addr, res->ai_addrlen);
        SOCKFUNC(freeaddrinfo)(result);
        return OsSocket::ipFromSockaddr(ss).ip;
      }
    }

    SOCKFUNC(freeaddrinfo)(result);

    return IpAddress();
  }

  int64_t getCurrentTime()
  {
    // TODO check if this is okay xD
    using namespace std::chrono;
    milliseconds ms =
        duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return ms.count();
  }


  HigherLayer* startTunTap(HusarnetManager* manager)
  {
    // yess
    return nullptr;
  }

  std::map<UserSetting, std::string> getEnvironmentOverrides()
  {
    // oh well, I copied it and it needs massaging.
    std::map<UserSetting, std::string> result;
    for(char** environ_ptr = environ; *environ_ptr != nullptr; environ_ptr++) {
      for(auto enumName : UserSetting::_names()) {
        auto candidate =
            "HUSARNET_" + strToUpper(camelCaseToUserscores(enumName));

        std::vector<std::string> splitted = split(*environ_ptr, '=', 1);
        if(splitted.size() == 1) {
          continue;
        }

        auto key = splitted[0];
        auto value = splitted[1];

        if(key == candidate) {
          result[UserSetting::_from_string(enumName)] = value;
          LOG("Overriding user setting %s=%s", enumName, value.c_str());
        }
      }
    }

    return result;

  }

  std::string readFile(std::string path)
  {
    // ympek TODO
    // copied from Unix implementation
    // but for some reason I need to add std::ifstream::in
    std::ifstream f(path);
    if(!f.good()) {
      LOG("failed to open %s", path.c_str());
      exit(1);
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    return buffer.str();
  }

  bool writeFile(std::string path, std::string content)
  {
    std::ofstream f(path, std::ofstream::out);
    if (!f.good()) {
      LOG("failed to write: %s", path.c_str());
      // hmm
      f.close();
      return false;
    }
    f << content;
    f.close();
    return true;
  }

  bool isFile(std::string path)
  {
    return PathFileExists(path.c_str());
  }

  void notifyReady()
  {

  }
}


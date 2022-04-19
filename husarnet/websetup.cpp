// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/websetup.h"
#include <iterator>
#include <sstream>
#include "husarnet/gil.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"
#include "husarnet/util.h"

WebsetupConnection::WebsetupConnection(HusarnetManager* manager)
    : manager(manager)
{
}

void WebsetupConnection::bind()
{
  websetupFd = SOCKFUNC(socket)(AF_INET6, SOCK_DGRAM, 0);
  assert(websetupFd > 0);
  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(WEBSETUP_SERVER_PORT);

  int ret = SOCKFUNC(bind)(websetupFd, (sockaddr*)&addr, sizeof(addr));
  assert(ret == 0);

  // this timeout is needed, so we can check initResponseReceived

#ifdef _WIN32
  int timeout = 2000;
#else
  timeval timeout{};
  timeout.tv_sec = 2;
#endif

  SOCKFUNC(setsockopt)
  (websetupFd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
}

void WebsetupConnection::init()
{
  sockaddr_in6* addr = (sockaddr_in6*)malloc(sizeof(sockaddr_in6));
  addr->sin6_family = AF_INET6;
  addr->sin6_port = htons(WEBSETUP_CLIENT_PORT);
  memcpy(&(addr->sin6_addr), manager->getWebsetupAddress().data.data(), 16);
  websetupAddr = (sockaddr*)addr;
}

void WebsetupConnection::send(
    std::string command,
    std::list<std::string> arguments)
{
  std::ostringstream ss;
  std::copy(
      arguments.begin(), arguments.end(),
      std::ostream_iterator<std::string>(ss, "\n"));
  const char* data = (command + "\n" + ss.str()).c_str();

  if(SOCKFUNC(sendto)(
         websetupFd, data, sizeof(data), 0, websetupAddr,
         sizeof(*websetupAddr)) < 0) {
    LOG("websetup UDP send failed (%d)", (int)errno);
  }
}

void WebsetupConnection::sendJoinRequest(
    std::string joinCode,
    std::string newWebsetupSecret,
    std::string selfHostname)
{
  send("init-request", {});
  send("init-request-join-code", {joinCode, newWebsetupSecret, selfHostname});
}

void WebsetupConnection::run()
{
  bind();
  while(true) {
    std::string buffer;
    buffer.resize(1024);
    sockaddr_in6 addr{};
    socklen_t addrsize = sizeof(addr);
    int ret = GIL::unlocked<int>([&]() {
      return SOCKFUNC(recvfrom)(
          websetupFd, &buffer[0], buffer.size(), 0, (sockaddr*)&addr,
          &addrsize);
    });

#ifdef _WIN32
    int err = WSAGetLastError();
    if(ret < 0 && (err == WSAETIMEDOUT || err == WSAECONNRESET))
      continue;
#else
    if(ret < 0 && (errno == EWOULDBLOCK || errno == EINTR))
      continue;
#endif
    assert(ret > 0 && addr.sin6_family == AF_INET6);

    auto sourceIp = IpAddress::fromBinary((char*)&addr.sin6_addr);
    if(sourceIp != manager->getWebsetupAddress()) {
      LOG("Websetup packet from invalid address: %s", sourceIp.str().c_str());
    }

    handleWebsetupPacket(buffer.substr(0, ret));
  }
}

void WebsetupConnection::handleWebsetupPacket(std::string data)
{
  std::string sharedSecret = manager->getWebsetupSecret();

  if(data.size() < sharedSecret.size() ||
     !NgSocketCrypto::safeEquals(
         data.substr(0, sharedSecret.size()), sharedSecret)) {
    LOG("invalid management shared secret");
    return;
  }

  data = data.substr(sharedSecret.size());

  long s = data.find("\n");
  if(s <= 5) {
    LOG("bad packet format");
    return;
  }
  std::string seqnum = data.substr(0, 5);
  std::string command = data.substr(5, s - 5);
  std::string payload = data.substr(s + 1);

  if(command == "ping") {
    LOGV("remote command: ping");
  } else {
    LOG("remote command: %s", command.c_str());
  }

  auto response = handleWebsetupCommand(command, payload);

  if(response.size() < 1) {
    return;
  }

  auto responseCommand = response.front();
  response.pop_front();
  auto responseArguments = response;

  send(seqnum + responseCommand, responseArguments);
}

std::list<std::string> WebsetupConnection::handleWebsetupCommand(
    std::string command,
    std::string payload)
{
  if(command == "ping") {
    return {"pong"};
  } else if(command == "init-response") {
    return {"ok"};
  } else if(command == "whitelist-add") {
    manager->whitelistAdd(IpAddress::parse(payload));
    return {"ok"};
  } else if(command == "whitelist-rm") {
    manager->whitelistRm(IpAddress::parse(payload));
    return {"ok"};
  } else if(command == "whitelist-enable") {
    manager->whitelistEnable();
    return {"ok"};
  } else if(command == "whitelist-disable") {
    manager->whitelistDisable();
    return {"ok"};
  } else if(command == "host-add") {
    auto parts = splitWhitespace(payload);
    if(parts.size() != 2) {
      return {"error"};
    }
    manager->hostTableAdd(parts[1], IpAddress::parse(parts[0]));
    return {"ok"};
  } else if(command == "host-rm") {
    auto parts = splitWhitespace(payload);
    if(parts.size() != 2) {
      return {"error"};
    }
    manager->hostTableRm(parts[1]);
    return {"ok"};
  } else if(command == "set-hostname") {
    manager->setSelfHostname(payload);
    return {"ok"};
  } else if(command == "change-secret") {
    manager->setWebsetupSecret(payload);
    return {"ok"};
  } else if(command == "get-version") {
    return {
        std::string("version=") + manager->getVersion() +
        ";ua=" + manager->getUserAgent()};
  } else if(command == "get-latency") {
    auto destination = IpAddress::parse(payload);
    auto latency = manager->getLatency(destination);
    return {"latency=" + std::to_string(latency)};
  } else if(command == "cleanup") {
    manager->cleanup();
    return {"ok"};
  }

  return {"bad-command"};
}

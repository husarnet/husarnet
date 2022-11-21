// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/websetup.h"

#include <array>

#include <assert.h>
#include <errno.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "husarnet/ports/port.h"

#include "husarnet/device_id.h"
#include "husarnet/gil.h"
#include "husarnet/husarnet_config.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/ipaddress.h"
#include "husarnet/logging.h"
#include "husarnet/ngsocket_crypto.h"
#include "husarnet/util.h"

#define WEBSETUP_CONTACT_TIMEOUT_MS (15 * 60 * 1000)

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

  SOCKFUNC(bind)(websetupFd, (sockaddr*)&addr, sizeof(addr));
  // TODO: we could probably handle the error and display some helpful info for
  // user e.g. EADRRINUSE -> you probably have Husarnet running... etc.

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

void WebsetupConnection::send(
    std::string command,
    std::list<std::string> arguments)
{
  send(
      InetAddress{manager->getWebsetupAddress(), WEBSETUP_CLIENT_PORT}, command,
      arguments);
}

void WebsetupConnection::send(
    InetAddress dstAddress,
    std::string command,
    std::list<std::string> arguments)
{
  std::string frame = "";
  frame.append(command);
  frame.append("\n");

  int i = 0;
  for(auto& it : arguments) {
    frame.append(it);
    if(i < (arguments.size() - 1)) {
      frame.append("\n");
    }
    i++;
  }

  sockaddr_in6 addr{};
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons(dstAddress.port);
  memcpy(&(addr.sin6_addr), dstAddress.ip.data.data(), 16);

  if(SOCKFUNC(sendto)(
         websetupFd, frame.data(), frame.size(), 0, (sockaddr*)&addr,
         sizeof(addr)) < 0) {
    LOG("websetup UDP send failed (%d)", (int)errno);
  }
}

Time WebsetupConnection::getLastContact()
{
  return lastContact;
}

Time WebsetupConnection::getLastInitReply()
{
  return lastInitReply;
}

void WebsetupConnection::periodicThread()
{
  while(true) {
    if(lastInitReply < (Port::getCurrentTime() - WEBSETUP_CONTACT_TIMEOUT_MS)) {
      if(joinCode.empty() || reportedHostname.empty()) {
        LOGV("Sending  update request to websetup");
        send("init-request", {});
      } else {
        LOGV("Sending join request to websetup");
        send(
            "init-request-join-code",
            {joinCode, manager->getWebsetupSecret(), reportedHostname});
      }
    }

    sleep(5);  // Remember that websetup has it's throttling mechanism
  }
}

void WebsetupConnection::handleConnectionThread()
{
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

    InetAddress replyAddress{sourceIp, htons(addr.sin6_port)};
    handleWebsetupPacket(replyAddress, buffer.substr(0, ret));
  }
}

void WebsetupConnection::start()
{
  bind();

  if(manager->getWebsetupSecret().empty()) {
    manager->setWebsetupSecret(encodeHex(generateRandomString(12)));
  }

  Port::startThread(
      [this]() { this->periodicThread(); }, "websetupPeriodic", 6000);

  GIL::startThread(
      [this]() { this->handleConnectionThread(); }, "websetupConnection", 6000);
}

void WebsetupConnection::handleWebsetupPacket(
    InetAddress replyAddress,
    std::string data)
{
  std::string sharedSecret = manager->getWebsetupSecret();

  if(data.size() < sharedSecret.size() ||
     !NgSocketCrypto::safeEquals(
         data.substr(0, sharedSecret.size()), sharedSecret)) {
    LOG("invalid management shared secret.\ngot: %s\nexp: %s",
        data.substr(0, sharedSecret.size()).c_str(), sharedSecret.c_str());
    LOG("message: %s", data.c_str());
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

  // Pings are too verbose - handle their logging separately
  if(command == "ping") {
    LOGV("remote command: ping");
  } else {
    LOG("remote command: %s, arguments: %s", command.c_str(), payload.c_str());
  }

  lastContact = Port::getCurrentTime();

  auto response = handleWebsetupCommand(command, payload);

  if(response.size() < 1) {
    return;
  }

  auto responseCommand = response.front();
  response.pop_front();
  auto responseArguments = response;

  send(replyAddress, seqnum + responseCommand, responseArguments);
}

std::list<std::string> WebsetupConnection::handleWebsetupCommand(
    std::string command,
    std::string payload)
{
  if(command == "ping") {
    return {"pong"};
  } else if(command == "init-response") {
    lastInitReply = Port::getCurrentTime();
    joinCode = "";  // mark that we've already joined
    return {"ok"};
  } else if(command == "whitelist-add") {
    manager->whitelistAdd(IpAddress::fromBinary(decodeHex(payload)));
    return {"ok"};
  } else if(command == "whitelist-rm") {
    manager->whitelistRm(IpAddress::fromBinary(decodeHex(payload)));
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
    auto destination = deviceIdFromIpAddress(IpAddress::parse(payload));
    auto latency = manager->getLatency(destination);
    return {"latency=" + std::to_string(latency)};
  } else if(command == "cleanup") {
    manager->cleanup();
    return {"ok"};
  }

  return {"bad-command"};
}

void WebsetupConnection::join(
    std::string joinCode,
    std::string reportedHostname)
{
  manager->setWebsetupSecret(encodeHex(generateRandomString(12)));

  this->joinCode = joinCode;
  this->reportedHostname = reportedHostname;

  lastInitReply = 0;  // initiate immediate join action
}

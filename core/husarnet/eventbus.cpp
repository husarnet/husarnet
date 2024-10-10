// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/eventbus.h"

#include "husarnet/http.h"
#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"

#include "etl/delegate.h"
#include "etl/string_view.h"

void EventBus::init()
{
  auto callback =
      WebSocket::OnMessageDelegate::create<EventBus, &EventBus::_handleMessage>(
          *this);
  this->ws.setOnMessageCallback(callback);
}

// Handle EB lifecycle
void EventBus::periodic()
{
  // Wait for join
  if(!this->manager->isJoined())
    return;

  // (Re)connect to the EventBus server
  if(ws.getState() == WebSocket::State::SOCK_CLOSED) {
    // Throttle connection attempts
    Time currentTime = Port::getCurrentTime();
    if(currentTime - this->_lastConnectionAttempt <
       CONNECTION_ATTEMPT_INTERVAL) {
      return;
    }

    this->_lastConnectionAttempt = currentTime;

    // Connect to EB WS with Husarnet address from the license file
    if(this->manager->getEbAddresses().size() == 0)
      return;

    // TODO: handle multiple EB addresses
    InetAddress ebAddress = {
        .ip = this->manager->getEbAddresses()[0],
        .port = 80,
    };

    std::string endpoint = "/device/" + this->manager->getSelfAddress().str();

    this->ws.connect(ebAddress, endpoint.data());
    // this->ws.connect(InetAddress::parse("127.0.0.1:8088"), "/");
  }
}

void EventBus::_handleMessage(WebSocket::Message& message)
{
  // Only text messages are supported
  if(message.opcode != WebSocket::Message::Opcode::TEXT)
    return;

  if(message.data.size() == 0)
    return;

  etl::string_view data(message.data.begin(), message.data.size());

  if(data.compare("getConfig") == 0) {
    this->_handleGetConfig();
  } else {
    LOG_WARNING("Unknown EB message: %s", data.data());
  }
}

void EventBus::_handleGetConfig()
{
  // Send config request to the EB
  etl::vector<char, 256> buffer;

  HTTPMessage http;
  http.request.method = "GET";
  http.request.endpoint = "/device/get_config";

  http.encode(buffer);

  // TODO: handle multiple EB addresses
  assert(this->manager->getDashboardApiAddresses().size() > 0);
  InetAddress ebAddress = {
      .ip = this->manager->getDashboardApiAddresses()[0],
      .port = 80,
  };

  int fd = OsSocket::connectUnmanagedTcpSocket(ebAddress);
  if(fd == -1) {
    LOG_ERROR("Failed to connect to EB");
    return;
  }

  int res = send(fd, buffer.data(), buffer.size(), 0);

  if(res == -1) {
    LOG_ERROR("Failed to send message to EB");
    close(fd);
    return;
  }

  // Receive response until the connection is closed
  // or a full HTTP message has been received
  HTTPMessage::Result httpResult = HTTPMessage::Result::INVALID;

  while(true) {
    buffer.clear();
    res = recv(fd, buffer.data(), buffer.capacity(), 0);

    if(res == -1) {
      LOG_ERROR("Failed to receive message from EB");
      close(fd);
      return;
    }

    if(res == 0) {
      LOG_INFO("EB connection closed");
      close(fd);
      return;
    }

    etl::string_view response(buffer.data(), res);
    httpResult = http.parse(response);

    // Exit on first parsed or failed message
    if(httpResult != HTTPMessage::Result::INCOMPLETE)
      break;
  }

  if(httpResult == HTTPMessage::Result::INVALID) {
    LOG_ERROR("Failed to receive message from EB");
    close(fd);
    return;
  }

  // Handle the received message
  LOG_INFO("Received message from EB");
  close(fd);
}

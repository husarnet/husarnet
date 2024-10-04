// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/eventbus.h"

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
    // if (this->manager->getEbAddresses().size() == 0)
    //   return;

    // // TODO: handle multiple EB addresses
    // InetAddress ebAddress = {
    //   .ip = this->manager->getEbAddresses()[0],
    //   .port = 80,
    // };

    // this->ws.connect(ebAddress, "/device/device_ws");
    this->ws.connect(InetAddress::parse("127.0.0.1:8088"), "/");
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
    // Send config
    this->ws.send("Hello from Husarnet daemon!");
  } else {
    LOG_WARNING("Unknown EB message: %s", data.data());
  }
}
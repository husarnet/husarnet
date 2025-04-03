// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/eventbus.h"

#include "husarnet/ports/port.h"

#include "husarnet/http.h"
#include "husarnet/logging.h"

#include "etl/delegate.h"
#include "etl/string_view.h"

EventBus::EventBus(HusarnetAddress myAddress, ConfigManager* configManager)
    : myAddress(myAddress), configManager(configManager)
{
}

void EventBus::init()
{
  auto callback = WebSocket::OnMessageDelegate::create<EventBus, &EventBus::_handleMessage>(*this);
  this->ws.setOnMessageCallback(callback);
}

// Handle EB lifecycle
void EventBus::periodic()
{
  // (Re)connect to the EventBus server
  if(ws.getState() == WebSocket::State::SOCK_CLOSED) {
    LOG_INFO("websocket is closed, attempt reconnectinon");
    // Throttle connection attempts
    Time currentTime = Port::getCurrentTime();
    if(currentTime - this->_lastConnectionAttempt < CONNECTION_ATTEMPT_INTERVAL) {
      return;
    }

    this->_lastConnectionAttempt = currentTime;

    const auto epIp = this->configManager->getEbAddress();
    // Connect to EB WS with Husarnet address from the license file
    if(!epIp.isFC94())
      return;

    InetAddress ebAddress = {
        .ip = epIp,
        .port = 80,
    };

    std::string endpoint = "/device/" + this->myAddress.toString();

    this->ws.connect(ebAddress, endpoint.data());
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

  if(data.compare("get_config") == 0) {
    this->configManager->triggerGetConfig();
  } else {
    LOG_WARNING("Unknown EB message: %s", data.data());
  }
}
// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <list>
#include <string>
#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

class HusarnetManager;

class WebsetupConnection {
  HusarnetManager* manager;

  Time lastContact = 0;
  int websetupFd;

  void bind();
  void send(
      InetAddress replyAddress,
      std::string command,
      std::list<std::string> arguments);
  void send(std::string command, std::list<std::string> arguments);
  void handleWebsetupPacket(InetAddress replyAddress, std::string data);
  std::list<std::string> handleWebsetupCommand(
      std::string command,
      std::string payload);

 public:
  WebsetupConnection(HusarnetManager* manager);
  void run();

  void sendJoinRequest(
      std::string joinCode,
      std::string newWebsetupSecret,
      std::string selfHostname);

  Time getLastContact();
};

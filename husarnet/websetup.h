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
  Time lastInitReply = 0;
  int websetupFd;

  // Join data
  std::string joinCode;
  std::string reportedHostname;

  void bind();
  void send(
      InetAddress replyAddress,
      std::string command,
      std::list<std::string> arguments);
  void sendJoinRequest();
  void send(std::string command, std::list<std::string> arguments);
  void handleWebsetupPacket(InetAddress replyAddress, std::string data);
  std::list<std::string> handleWebsetupCommand(
      std::string command,
      std::string payload);

  void sendInit();
  void periodicThread();
  void handleConnectionThread();

 public:
  WebsetupConnection(HusarnetManager* manager);
  void start();

  void join(std::string joinCode, std::string reportedHostname);
  Time getLastContact();
};

#pragma once
#include <list>
#include <string>
#include "port.h"
#include "sockets.h"

class HusarnetManager;

class WebsetupConnection {
  HusarnetManager* manager;

  int websetupFd;
  sockaddr* websetupAddr;

  void bind();
  void send(std::string command, std::list<std::string> arguments);
  void handleWebsetupPacket(std::string data);
  std::list<std::string> handleWebsetupCommand(std::string command,
                                               std::string payload);

 public:
  WebsetupConnection(HusarnetManager* manager);
  void init();
  void run();

  void sendJoinRequest(std::string joinCode,
                       std::string newWebsetupSecret,
                       std::string selfHostname);
};

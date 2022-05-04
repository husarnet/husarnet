// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/ngsocket.h"

class TunDelegate : private NgSocketDelegate {
  NgSocket* sock;
  int tunFd = -1;
  std::string tunBuffer;

  void onDataPacket(DeviceId source, string_view data);
  void tunReady();
  TunDelegate(NgSocket* netDev);

 public:
  void setFd(int tunFd);
  void closeFd();
  bool isRunning();

  static TunDelegate*
  startTun(std::string name, NgSocket* netDev, bool isTap = false);
  static TunDelegate* createTun(NgSocket* netDev);
};

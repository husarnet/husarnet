// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <mutex>

#include "husarnet/ports/port.h"

#include "husarnet/util.h"

#include "husarnet.h"

class WinTap {
  void* tap_fd;
  std::string deviceName;

 public:
  static WinTap* create(std::string savedDeviceName);
  std::string getDeviceName() { return deviceName; }
  std::string getNetshName();
  void bringUp();
  std::string getMac();
  string_view read(std::string& buffer);
  void write(string_view data);
};

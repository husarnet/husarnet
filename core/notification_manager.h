// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>

#include <sodium.h>

#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/periodic_timer.h"
#include "husarnet/util.h"

#include "nlohmann/json.hpp"

class NotificationManagerInterface {
 public:
  virtual ~NotificationManagerInterface() = default;

  virtual std::list<std::string> getNotifications() = 0;
};

class NotificationManager : public NotificationManagerInterface {
 public:
  NotificationManager(
      std::string dashboardHostname,
      HusarnetManager* husarnetManager);
  ~NotificationManager();
  
  virtual std::list<std::string> getNotifications();

 private:
  PeriodicTimer* timer;
  std::chrono::hours interval;
  std::string static dashboardHostname;
  static HusarnetManager* husarnetManager;
  void static refreshNotificationFile();
  json static retrieveNotificationJson(std::string dashboardHostname);
};

class DummyNotificationManager : public NotificationManagerInterface {
 public:
  DummyNotificationManager() {};

  virtual std::list<std::string> getNotifications()
  {
    return {};
  }
};

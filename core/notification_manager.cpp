// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/notification_manager.h"

std::string NotificationManager::dashboardHostname;
HusarnetManager* NotificationManager::husarnetManager;

NotificationManager::NotificationManager(
    std::string dashboardHostname_,
    HusarnetManager* husarnetManager_)
{
  this->interval = std::chrono::hours{12};
  dashboardHostname = dashboardHostname_;
  husarnetManager = husarnetManager_;
  refreshNotificationFile();
  this->timer = new PeriodicTimer(interval, refreshNotificationFile);
  this->timer->Start();
};
NotificationManager::~NotificationManager()
{
  delete this->timer;
}

void NotificationManager::refreshNotificationFile()
{
 // This may look a bit confusing but in order to solve several edge cases with
 // custom time and date settings on client systems and in order to handle data
 // loss as gracefully as possible this kind of solutions was the best apparent
 // one.
 // We expect to get new notifications for dashboard in a json containing a
 // list of objects, where each object has id,
 // content of the notification and a validity span (in days). On the client
 // side we store the the latest_id we got (and when looking at notifications
 // we got from dashboard we ignore those with lower or equal id to this one),
 // we also store notifications that are still walid and should be displayed
 //(we store content of the notification and a time point casted to int once
 // the time point is exceeded this notification will be filtered out on
 // refresh) we utilize client system clock to calculate the validity time point
 // and only specify timespan on the server side this mechanism ensures that
 // even if client system is configured with a custom date and time,
 // notifications will work as if the time and date was the same as on the
 // server side. The id mechanism also ensures that individual notifications
 // will be displayed to users for the same time spans independently of the
 // time the client received the notification.

  auto new_notifications = retrieveNotificationJson(dashboardHostname);
  auto cached = Privileged::readNotificationFile();
  json merged;
  merged["notifications"] = json::array();
  merged["latest_id"] = 0;
  int now = static_cast<int>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
  if(cached != "{}" and new_notifications.contains("notifications")) {
    auto notifications = nlohmann::json::parse(cached);
    if(notifications.contains("latest_id") and
       notifications.contains("notifications")) {
      merged["latest_id"] = notifications["latest_id"];
      for(const auto& element : notifications["notifications"]) {
        if(element["valid_until"].get<int>() >= now) {
          merged["notifications"].push_back(element);
        }
      }
    }
    for(const auto& element : new_notifications["notifications"]) {
      if(element["id"] > merged["latest_id"]) {
        merged["latest_id"] = element["id"];
        json j;
        j.emplace("content", element["content"]);
        j.emplace(
            "valid_until",
            now + element["valid_for"].get<int>() * dayTimeout);
        merged["notifications"].push_back(j);
      }
    }

  } else if(new_notifications.contains("notifications")) {
    for(const auto& element : new_notifications["notifications"]) {
      if(element["id"] > merged["latest_id"]) {
        merged["latest_id"] = element["id"];
        json j;
        j.emplace("content", element["content"]);
        j.emplace(
            "valid_until",
            now + element["valid_for"].get<int>() * dayTimeout);
        merged["notifications"].push_back(j);
      }
    }

  } else if(cached != "{}") {
    auto notifications = nlohmann::json::parse(cached);
    if(notifications.contains("latest_id") and
       notifications.contains("notifications")) {
      merged["latest_id"] = notifications["latest_id"];
      for(const auto& element : notifications["notifications"]) {
        if(element["valid_until"].get<int>() >= now) {
          merged["notifications"].push_back(element);
        }
      }
    }
  } else {
    merged.clear();
  }

  // Merged cached and requested notifications
  husarnetManager->getHooksManager()->withRw(
      [&]() { Privileged::writeNotificationFile(merged.dump()); });
};

std::list<std::string> NotificationManager::getNotifications()
{
  auto cached = Privileged::readNotificationFile();
  if(cached == "{}")
    return {};
  auto notifications = nlohmann::json::parse(cached);
  std::list<std::string> ls{};
  int now = static_cast<int>(
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
  if(notifications.contains("notifications")) {
    for(const auto& element : notifications["notifications"]) {
      if(element["valid_until"].get<int>() >= now) {
        ls.push_back(element["content"].get<std::string>());
      }
    }
  }

  return ls;
};

json NotificationManager::retrieveNotificationJson(
    std::string dashboardHostname)
{
  IpAddress ip = Privileged::resolveToIp(dashboardHostname);
  InetAddress address{ip, 80};
  int sockfd = OsSocket::connectTcpSocket(address);
  if(sockfd < 0) {
    LOG_ERROR(
        "Can't contact %s - is DNS resolution working properly?",
        dashboardHostname.c_str());
    return json::parse("{}");
  }

  std::string readBuffer;
  readBuffer.resize(8192);

  std::string request =
      "GET /announcements.json HTTP/1.1\r\n"
      "Host: " +
      dashboardHostname +
      "\r\n"
      "User-Agent: husarnet\r\n"
      "Accept: */*\r\n\r\n";

  SOCKFUNC(send)(sockfd, request.data(), request.size(), 0);
  size_t len =
      SOCKFUNC(recv)(sockfd, (char*)readBuffer.data(), readBuffer.size(), 0);
  size_t pos = readBuffer.find("\r\n\r\n");

  if(pos == std::string::npos) {
    LOG_ERROR("invalid response from the server: %s", readBuffer.c_str());
    abort();
  }
  pos += 4;
  return json::parse(readBuffer.substr(pos, len - pos));
}

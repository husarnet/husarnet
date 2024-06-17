// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <map>
#include <vector>

#include "husarnet/config_storage.h"
#include "husarnet/ipaddress.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#define HUSARNET_VERSION "2.0.239"
#define WEBSETUP_SERVER_PORT 5580
#define WEBSETUP_CLIENT_PORT 4800
#define BASESERVER_PORT 443
#define MULTICAST_PORT 5581

static const int MAX_QUEUED_PACKETS = 10;

// this is IPv4 AD-HOC multicast address (224.3.252.148)
const IpAddress MULTICAST_ADDR_4 = IpAddress::parse("::ffff:E003:FC94");
const IpAddress MULTICAST_ADDR_6 =
    IpAddress::parse("ff02:88bb:31e4:95f7:2b87:6b52:e112:19ac");

const IpAddress multicastDestination = IpAddress::parse("ff15:f2d3:a389::");

const std::map<UserSetting, std::string> userDefaults = {
    {UserSetting::dashboardFqdn, "app.husarnet.com"},
    {UserSetting::enableWhitelist, trueValue},
    {UserSetting::interfaceName, "hnet0"},
    {UserSetting::daemonApiPort, "16216"},
    {UserSetting::enableCompression, falseValue},
    {UserSetting::enableUdpTunelling, trueValue},
    {UserSetting::enableTcpTunelling, trueValue},
    {UserSetting::enableUdp, trueValue},
    {UserSetting::enableMulticast, trueValue},
    {UserSetting::overrideBaseAddress, ""},
    {UserSetting::overrideSourcePort, ""},
    {UserSetting::extraAdvertisedAddress, ""},
    {UserSetting::joinCode, ""},
    {UserSetting::hostname, ""},
    {UserSetting::enableHooks, falseValue},
    {UserSetting::daemonApiAddress, "127.0.0.1"},
    {UserSetting::daemonApiInterface, ""},
};

const std::map<InternalSetting, std::string> internalDefaults = {
    {InternalSetting::websetupSecret, ""},
};

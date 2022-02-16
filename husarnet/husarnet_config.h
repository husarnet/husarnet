// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#pragma once
#include <map>
#include <vector>
#include "config_storage.h"
#include "ipaddress.h"

#define HUSARNET_VERSION "2021.12.20.2"
#define WEBSETUP_SERVER_PORT 5580
#define WEBSETUP_CLIENT_PORT 4800

static const int MAX_QUEUED_PACKETS = 10;

// this is IPv4 AD-HOC multicast address (224.3.252.148)
const IpAddress MULTICAST_ADDR_4 = IpAddress::parse("::ffff:E003:FC94");
const IpAddress MULTICAST_ADDR_6 =
    IpAddress::parse("ff02:88bb:31e4:95f7:2b87:6b52:e112:19ac");

const std::map<UserSetting, std::string> userDefaults = {};
const std::map<InternalSetting, std::string> internalDefaults = {};

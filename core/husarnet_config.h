// Copyright (c) 2017 Husarion Sp. z o.o.
// author: Michał Zieliński (zielmicha)
#include "ipaddress.h"
#include <vector>

#define HUSARNET_VERSION "2021.11.17.2"

__attribute__((unused)) static const InetAddress baseTcpAddresses[] = {
    {IpAddress::parse("188.165.23.196"), 443}, // ovh RBX (base-2)
    {IpAddress::parse("147.135.76.110"), 443}, // ovh US (base-3)
    {IpAddress::parse("51.254.25.193"), 443},  // fallback server
};

__attribute__((unused)) static const char *dashboardUrl = "https://app.husarnet.com";
__attribute__((unused)) static const char *dashboardHostname = "app.husarnet.com";
__attribute__((unused)) static const char *baseDnsAddress = "base.husarnet.com";

static const int MAX_QUEUED_PACKETS = 10;

__attribute__((unused)) static const char *defaultWebsetupHosts[] = {
    "fc94:7d8a:8254:8e9b:7e54:ad88:4540:cae",  // websetup-id-1
    "fc94:50c:e453:ef91:203c:6ef2:3f9b:8145",  // websetup-id-2
    "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a", // websetup-id-3
    "fc94:977a:9af5:bcd8:efff:da4c:c86d:3195", // websetup-id-4 (husarnet.com)
};

__attribute__((unused)) static const char *defaultJoinHost = "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a";

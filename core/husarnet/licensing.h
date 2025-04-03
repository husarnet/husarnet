// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once

#include "nlohmann/json.hpp"

using namespace nlohmann;  // json

#define LICENSE_VERSION_KEY "version"
#define LICENSE_INSTALLATION_ID_KEY "installation_id"
#define LICENSE_LICENSE_ID_KEY "license_id"
#define LICENSE_NAME_KEY "name"
#define LICENSE_MAX_DEVICES_KEY "max_devices"
#define LICENSE_DASHBOARD_URL_KEY "dashboard_url"
#define LICENSE_WEBSETUP_HOST_KEY "websetup_host"
#define LICENSE_BASE_SERVER_ADDRESSES_KEY "base_server_addresses"
#define LICENSE_ISSUED_KEY "issued"
#define LICENSE_VALID_UNTIL_KEY "valid_until"
#define LICENSE_API_SERVERS_KEY "api_servers"
#define LICENSE_EB_SERVERS_KEY "eb_servers"
#define LICENSE_SIGNATURE_KEY "signature"
#define LICENSE_SIGNATURE_V2_KEY "signature_v2"

bool isLicenseValid(const json& licenseJson);

#pragma once
#include <list>
#include <string>
#include "../ipaddress.h"

namespace Privileged {
void init();

std::string updateLicenseFile();

std::string getSelfHostname();
void changeSelfHostname(std::string newHostname);

void updateHostsFile(std::list<std::pair<std::string, IpAddress>> data);

void saveLicenseFile(std::string contents);
std::string readLicenseFile();
}  // namespace Privileged

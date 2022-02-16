#pragma once
#include <list>
#include <string>
#include "../ipaddress.h"

namespace Privileged {
void init();   // Should be called as early as possible
void start();  // Should be called as soon as the parent process is ready to
               // drop all privileges

std::string readSettings();
void writeSettings(std::string);

// std::string updateLicenseFile();

// std::string getSelfHostname();
// void changeSelfHostname(std::string newHostname);

// void updateHostsFile(std::list<std::pair<std::string, IpAddress>> data);

// void saveLicenseFile(std::string contents);
// std::string readLicenseFile();
}  // namespace Privileged

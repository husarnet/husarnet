// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// Support for editing /etc/hosts.
#include <fstream>
#include "port.h"
#include "service_helper.h"
#include "util.h"

namespace ServiceHelper {

  const std::string marker = " # managed by Husarnet";

  bool isLineMarked(std::string line) { return endswith(line, marker); }

  void parseHostsFile(
      std::ifstream& input,
      std::string& managedLines,
      std::string& unmanagedLines)
  {
    managedLines = "";
    unmanagedLines = "";
    std::string fline;

    while(std::getline(input, fline)) {
      if(isLineMarked(fline)) {
        managedLines += fline + "\n";
      } else {
        unmanagedLines += fline + "\n";
      }
    }
  }

  std::string makeHostsFileEntries(
      std::vector<std::pair<IpAddress, std::string>> data)
  {
    std::string lines;
    for(auto p : data) {
      if(validateHostname(p.second))
        lines += p.first.str() + " " + p.second + marker + "\n";
    }
    return lines;
  }

  std::string getHostsFilePath()
  {
#ifdef _WIN32
    std::string hostsFile =
        std::string(getenv("windir")) + "/system32/drivers/etc/hosts";
#else
    std::string hostsFile = "/etc/hosts";
    if(getenv("HUSARNET_HOSTS_FILE"))
      hostsFile = getenv("HUSARNET_HOSTS_FILE");
#endif
    return hostsFile;
  }

  std::vector<std::pair<IpAddress, std::string>> getCurrentHostsFileInternal()
  {
    std::vector<std::pair<IpAddress, std::string>> result;
    std::string hostsFile = getHostsFilePath();
    std::ifstream input(hostsFile);
    if(!input.good()) {
      LOG("cannot open /etc/hosts");
      return {};
    }

    std::string fline;
    while(std::getline(input, fline)) {
      if(isLineMarked(fline)) {
        auto s = splitWhitespace(fline);
        if(validateHostname(s[1]))
          result.push_back({IpAddress::parse(s[0]), s[1]});
      }
    }

    return result;
  }

  bool updateHostsFileInternal(
      std::vector<std::pair<IpAddress, std::string>> data)
  {
    std::string hostsFile = getHostsFilePath();
    std::string tmpname = hostsFile + ".tmp";

    std::ifstream input(hostsFile);
    if(!input.good()) {
      LOG("cannot open /etc/hosts");
      return false;
    }

    std::string unmanagedLines;
    std::string originalManagedLines;
    parseHostsFile(input, originalManagedLines, unmanagedLines);

    std::string newManagedLines = makeHostsFileEntries(data);

    if(originalManagedLines == newManagedLines)
      return true;

    std::string result = unmanagedLines + newManagedLines;

    input.close();
    std::ofstream out(tmpname, std::ofstream::trunc);
    if(!out.good()) {
      LOG("cannot overwrite /etc/hosts");
      return false;
    }

    out << result;
    out.close();

    // Note: renameOrCopyFile will leave the tmp file on the filesystem if
    // rename is not possible but as it's name is stable this shouldn't do much
    // harm
    int ret = renameOrCopyFile(tmpname.c_str(), hostsFile.c_str());
    if(ret != 0) {
      LOG("cannot overwrite hosts file (%s) (code: %i)", hostsFile.c_str(),
          ret);
      return false;
    }

    return true;
  }
}  // namespace ServiceHelper

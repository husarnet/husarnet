// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <iostream>
#include <string>

#include "husarnet/ports/port.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/util.h"

void printUsage()
{
  std::cout << "Usage: husarnet-daemon (--version|--genid)" << std::endl;
}

int main(int argc, const char* argv[])
{
  if(argc == 1) {
    auto* manager = new HusarnetManager();
    manager->prepareHusarnet();
    manager->runHusarnet();

    return 0;  // this will technically never happen as runHusarnet is blocking
               // but it looks cleaner
  }

  if(argc == 2) {
    std::string argument(argv[1]);

    if(argument == "-v" || argument == "--version" || argument == "version") {
      std::cout << HUSARNET_VERSION << std::endl;
      return 0;
    }

    if(argument == "--genid" || argument == "genid") {
      std::cout << Identity::create().serialize();
      return 0;
    }

    printUsage();
    return 3;
  }

  if(argc > 2) {
    printUsage();
    return 2;
  }

  std::cout << "Please use husarnet CLI for managing the daemon. The daemon "
               "itself requires no arguments to run. In order to check "
               "version run husarnet-daemon --version."
            << std::endl;

  return 1;
}

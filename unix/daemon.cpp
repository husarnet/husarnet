// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <iostream>
#include <string>

#include "husarnet/ports/port.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/util.h"

int main(int argc, const char* argv[])
{
  if(argc == 1) {
    auto* manager = new HusarnetManager();
    manager->runHusarnet();
    return 0;  // this will technically never happen as runHusarnet is blocking
               // but it looks cleaner
  }

  if(argc == 2) {
    std::string argument(argv[1]);
    if(argument == "-v" || argument == "--version" || argument == "version") {
      std::cout << HUSARNET_VERSION << std::endl;
      return 1;
    }
  }

  std::cout << "Please use husarnet CLI for managing the daemon. The daemon "
               "itself requires no arguments to run. In order to check "
               "version run husarnet-daemon --version."
            << std::endl;

  return 2;
}

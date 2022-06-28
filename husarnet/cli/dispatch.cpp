// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/cli/dispatch.h"

#include <exception>
#include <iostream>
#include <string>

#include "husarnet/cli/api_client.h"
#include "husarnet/cli/parser.h"
#include "husarnet/husarnet_manager.h"

#include "httplib.h"

void dispatchCli(CLIOpts opts)
{
  // This is a special case as it does not use any other CLI facilities like
  // HTTP client
  if(opts.command == Command::daemon) {
    auto* manager = new HusarnetManager();
    manager->runHusarnet();
    return;
  }

  try {
    auto client = prepareApiClient();

    switch(opts.command) {
      case Command::daemon:
        // This was handled above. Here's a copy to make linter happy
        break;
      case Command::status:
        std::cout << apiGet(client, "/control/status")->body << std::endl;
        break;
      case Command::join:
        std::cout
            << apiPost(client, "/control/join", {{"code", opts.joincode}})->body
            << std::endl;
        break;
      case Command::setup_server:
        std::cout << apiPost(
                         client, "/control/change-server",
                         {{"domain", opts.address}})
                         ->body
                  << std::endl;
        break;
      case Command::whitelist:
        switch(opts.subcommand) {
          case Subcommand::enable:
            std::cout << apiPost(client, "/control/whitelist/enable")->body
                      << std::endl;
            break;
        }
        break;
    }
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
  } catch(const std::string& e) {
    std::cerr << e << std::endl;
  }
}

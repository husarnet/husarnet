#include "cli/dispatch.h"
#include <iostream>
#include "cli/api_client.h"
#include "husarnet_manager.h"
#include "ports/unix/service.h"

void dispatchCli(CLIOpts opts) {
  // This is a special case as it does not use any other CLI facilities like
  // HTTP client
  if (opts.command == Command::daemon) {
    auto* manager = new HusarnetManager();
    manager->runHusarnet();
    return;
  }

  try {
    auto client = prepareApiClient();

    switch (opts.command) {
      case Command::daemon:
        // This was handled above. Here's a copy to make linter happy
        break;
      case Command::status:
        std::cout << apiGet(client, "/control/status")->body << std::endl;
        break;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  } catch (const std::string& e) {
    std::cerr << e << std::endl;
  }
}

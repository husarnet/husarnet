// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/port_interface.h"
#include "husarnet/ports/sockets.h"

namespace Port 
{
  void processSocketEvents(HusarnetManager* manager)
  {
    OsSocket::runOnce(1000);  // process socket events for at most so many ms
  }
}
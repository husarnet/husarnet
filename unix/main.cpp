// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <iostream>
#include <string>

#include "husarnet/ports/port.h"

#include "husarnet/cli/dispatch.h"
#include "husarnet/cli/parser.h"

int main(int argc, const char* argv[])
{
  CLIOpts opts = parseCliArgs(argc, argv);
  dispatchCli(opts);

  return 0;
}

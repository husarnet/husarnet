// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// port.h has to be the first include, newline afterwards prevents autoformatter
// from sorting it
#include "port.h"

#include "dispatch.h"
#include "parser.h"

#include <iostream>
#include <string>

int main(int argc, const char* argv[]) {
  CLIOpts opts = parseCliArgs(argc, argv);
  dispatchCli(opts);

  return 0;
}

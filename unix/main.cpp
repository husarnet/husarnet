// port.h has to be the first include, newline afterwards prevents autoformatter
// from sorting it
#include "port.h"

#include "dispatch.h"
#include "parser.h"

#include <iostream>
#include <string>

int main(int argc, const char* argv[]) {
  initPort();

  CLIOpts opts = parseCliArgs(argc, argv);
  dispatchCli(opts);

  return 0;
}

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "parser.h"
#include <iostream>
#include "docopt.h"
#include "husarnet_config.h"

static const char USAGE[] =
    R"(Husarnet Client

Usage:
  husarnet join <joincode> [<hostname>]
  husarnet status [--json]
  husarnet wait [--daemon] [--base]
  husarnet whitelist [ls]
  husarnet whitelist (enable | disable)
  husarnet whitelist (add | rm) <address>
  husarnet host [ls]
  husarnet host (add | rm) <hostname> <address>
  husarnet setup-server <address>
  husarnet websetup
  husarnet daemon [--background]
  husarnet logs [--set-size=<size>] [--set-level=<level>]
  husarnet (-h | --help)
  husarnet --version

Commands:
  join          Connect to Husarnet Network
  status        Get Husarnet Client daemon status (in JSON format if --json provided)
  wait          Wait until the Daemon and/or Base Server connection is ready (defaults to both)
  whitelist     Manage the whitelist (by default whitelist is enabled)
  host          Manage the hosts table
  setup-server  Connect Client to another Stack (only one Stack is allowed at any given time) (please restart the daemon afterwards) (defaults to app.husarnet.com)
  websetup      Connect to Husarnet Network using browser
  daemon        Husarnet Client daemon (actual daemon) (usually started automatically with systemd) (there should be no need to run it manually)
  logs          View and manage daemon's logs

Options:
  -h --help     Show this screen.
  --version     Show version.
)";

CLIOpts parseCliArgs(int argc, const char* argv[])
{
  std::map<std::string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, HUSARNET_VERSION);

  // for (auto const& arg : args) {
  //   std::cout << arg.first << ":" << arg.second << std::endl;
  // }

  CLIOpts opts;

  // You can get away with using operator[] with no sanity checks here
  // as all keys will be generated by docopt

  if(args["join"].asBool()) {
    opts.command = Command::join;
  } else if(args["status"].asBool()) {
    opts.command = Command::status;
  } else if(args["wait"].asBool()) {
    opts.command = Command::wait;
  } else if(args["whitelist"].asBool()) {
    opts.command = Command::whitelist;
  } else if(args["host"].asBool()) {
    opts.command = Command::host;
  } else if(args["setup-server"].asBool()) {
    opts.command = Command::setup_server;
  } else if(args["daemon"].asBool()) {
    opts.command = Command::daemon;
  } else if(args["logs"].asBool()) {
    opts.command = Command::logs;
  }

  if(args["ls"].asBool()) {
    opts.subcommand = Subcommand::ls;
  } else if(args["enable"].asBool()) {
    opts.subcommand = Subcommand::enable;
  } else if(args["disable"].asBool()) {
    opts.subcommand = Subcommand::disable;
  } else if(args["add"].asBool()) {
    opts.subcommand = Subcommand::add;
  } else if(args["rm"].asBool()) {
    opts.subcommand = Subcommand::rm;
  }

  opts.joincode =
      args["<joincode>"] ? args["<joincode>"].asString() : std::string();
  opts.address =
      args["<address>"] ? args["<address>"].asString() : std::string();
  opts.hostname =
      args["<hostname>"] ? args["<hostname>"].asString() : std::string();

  opts.json = args["--json"].asBool();
  opts.daemon = args["--daemon"].asBool();
  opts.base = args["--base"].asBool();
  opts.background = args["--background"].asBool();

  // Those bool conversions test whether an argument is present
  opts.set_size = bool(args["--set-size"]);
  opts.size = args["--set-size"] ? args["--set-size"].asLong() : 0;
  opts.set_level = bool(args["--set-level"]);
  opts.level = args["--set-size"] ? args["--set-level"].asLong() : 0;

  return opts;
}

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

enum class Command {
  join,
  status,
  wait,
  whitelist,
  host,
  setup_server,
  websetup,
  daemon,
  logs
};
enum class Subcommand {
  ls,
  enable,
  disable,
  add,
  rm,
};

class CLIOpts {
 public:
  Command command;
  Subcommand subcommand;

  std::string joincode;
  std::string address;
  std::string hostname;

  bool json;
  bool daemon;
  bool base;
  bool background;

  bool set_size;
  int size;
  bool set_level;
  int level;
};

CLIOpts parseCliArgs(int argc, const char* argv[]);

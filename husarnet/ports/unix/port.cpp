// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <thread>
#ifndef __ANDROID__
#include <linux/random.h>
#include <syscall.h>
#endif
#include <signal.h>
#include <stdio.h>

#include <ares.h>

#include "husarnet/gil.h"
#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"
#include "husarnet/util.h"

bool husarnetVerbose = true;

// TODO remove this file

bool writeFile(std::string path, std::string data) {
  FILE* f = fopen((path + ".tmp").c_str(), "wb");
  int ret = fwrite(data.data(), data.size(), 1, f);
  if (ret != 1) {
    LOG("could not write to %s (failed to write to temporary file)",
        path.c_str());
    return false;
  }
  fsync(fileno(f));
  fclose(f);

  if (rename((path + ".tmp").c_str(), path.c_str()) < 0) {
    LOG("could not write to %s (rename failed)", path.c_str());
    return false;
  }
  return true;
}

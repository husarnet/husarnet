// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "husarnet/ports/dummy_task_priorities.h"

#include "tun.h"

#define SOCKFUNC(name) ::name
#define SOCKFUNC_close SOCKFUNC(close)

#define ENABLE_IPV6

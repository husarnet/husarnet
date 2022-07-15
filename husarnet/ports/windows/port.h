// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cassert>
#include <cstdlib>
#ifdef _WIN32
// #define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <mswsock.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winioctl.h>
#undef IGNORE  // ...
#endif
#include <string>

extern errno_t rand_s(unsigned int* randomValue);
inline void sleep(int sec)
{
  Sleep(sec * 1000);
}
#define SOCKFUNC(name) ::name
#define SOCKFUNC_close closesocket

inline long random()
{
  unsigned int res = 0;
  auto ok = rand_s(&res);
  assert(ok == 0);
  return res;
}

const char* getThreadName();

inline int renameFile(const char* src, const char* dst)
{
  return MoveFileEx(src, dst, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
}

void fixPermissions(std::string path);

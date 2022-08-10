// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"

#include "husarnet/fstring.h"
#include "husarnet/logmanager.h"
#include "husarnet/string_view.h"

#ifdef __ANDROID__
#include <android/log.h>

#define LOG(msg, x...)                                 \
  __android_log_print(                                 \
      ANDROID_LOG_INFO, "husarnet", "[%ld] " msg "\n", \
      (long int)Port::getCurrentTime(), ##x)
#define LOGV(msg, x...) LOG(msg, ##x)

#else
#ifdef _WIN32
#define LOG(msg, x...)                                                         \
  do {                                                                         \
    fprintf(                                                                   \
        stderr, "[%lld] %s: " msg "\n", (long long int)Port::getCurrentTime(), \
        Port::getThreadName(), ##x);                                           \
    fflush(stderr);                                                            \
  } while(0)
#else
#define LOG(msg, x...)                                                     \
  do {                                                                     \
    fprintf(                                                               \
        stderr, "[%lld] " msg "\n", (long long int)Port::getCurrentTime(), \
        ##x);                                                              \
    fflush(stderr);                                                        \
    if(globalLogManager != nullptr) {                                      \
      char buf[1024];                                                      \
      snprintf(                                                            \
          buf, 1024, "[%lld] " msg "\n",                                   \
          (long long int)Port::getCurrentTime(), ##x);                     \
      globalLogManager->insert(buf);                                       \
    }                                                                      \
  } while(0)
#endif
// globalLogManager->insert("["+std::to_string((long long
// int)Port::getCurrentTime())+"]
// "+msg);
#define LOGV1(msg, x...)                          \
  do {                                            \
    if(globalLogManager != nullptr) {             \
      if(globalLogManager->getVerbosity() >= 1) { \
        LOG(msg, ##x);                            \
      }                                           \
    } else {                                      \
      LOG(msg, ##x);                              \
    }                                             \
  } while(0)
#define LOGV2(msg, x...)                          \
  do {                                            \
    if(globalLogManager != nullptr) {             \
      if(globalLogManager->getVerbosity() >= 2) { \
        LOG(msg, ##x);                            \
      }                                           \
    } else {                                      \
      LOG(msg, ##x);                              \
    }                                             \
  } while(0)
#define LOGV(msg, x...)                           \
  do {                                            \
    if(globalLogManager != nullptr) {             \
      if(globalLogManager->getVerbosity() >= 3) { \
        LOG(msg, ##x);                            \
      }                                           \
    } else {                                      \
      LOG(msg, ##x);                              \
    }                                             \
  } while(0)
#endif

#define LOG_DEBUG(msg, x...)  // LOG(msg, ##x)

#ifdef ESP_PLATFORM
inline const char*
memmem(const char* stack, int slen, const char* needle, int nlen)
{
  if(nlen > slen)
    return nullptr;

  for(int i = 0; i <= slen - nlen; i++) {
    if(memcmp(stack + i, needle, nlen) == 0)
      return stack + i;
  }
  return nullptr;
}
#endif

template <typename Vec, typename T>
bool insertIfNotPresent(Vec& v, const T& t)
{
  if(std::find(v.begin(), v.end(), t) == v.end()) {
    v.push_back(t);
    return true;
  } else {
    return false;
  }
}

template <typename T>
std::string pack(T t)
{
  std::string s;
  s.resize(sizeof(T));
  memcpy(&s[0], &t, sizeof(T));
  return s;
}

template <typename T>
void packTo(T t, void* dst)
{
  memcpy(dst, &t, sizeof(T));
}

template <typename T>
T unpack(std::string s)
{
  if(s.size() != sizeof(T))
    abort();
  T r;
  memcpy(&r, s.data(), sizeof(T));
  return r;
}

template <typename T>
T unpack(fstring<sizeof(T)> s)
{
  T r;
  memcpy(&r, s.data(), sizeof(T));
  return r;
}

static const char* hexLetters = "0123456789abcdef";

inline std::string encodeHex(std::string s)
{
  std::string ret;
  for(unsigned char ch : s) {
    ret.push_back(hexLetters[(ch >> 4) & 0xF]);
    ret.push_back(hexLetters[ch & 0xF]);
  }
  return ret;
}

inline std::string decodeHex(std::string s)
{
  if(s.size() % 2 != 0)
    return "";

  std::string ret;
  for(int i = 0; i + 1 < (int)s.size(); i += 2) {
    int a = (int)(std::find(hexLetters, hexLetters + 16, s[i]) - hexLetters);
    int b =
        (int)(std::find(hexLetters, hexLetters + 16, s[i + 1]) - hexLetters);
    if(a == 16 || b == 16)
      return "";
    ret.push_back((char)((a << 4) | b));
  }
  return ret;
}

std::string base64Decode(std::string s);

inline bool startswith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(0, with.size()) == with;
}

inline bool endswith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(s.size() - with.size()) == with;
}

std::vector<std::string> splitWhitespace(std::string s);
std::vector<std::string> split(std::string s, char byChar, int maxSplit);

std::pair<bool, int> parse_integer(std::string s);

template <typename A, typename B>
struct pair_hash {
  unsigned operator()(const std::pair<A, B>& a) const
  {
    return std::hash<A>()(a.first) + std::hash<B>()(a.second) * 23456213;
  }
};

std::string generateRandomString(const int length);
std::string strToUpper(std::string input);
std::string strToLower(std::string input);
std::string rtrim(std::string input);
std::string camelCaseToUnderscores(std::string camel);

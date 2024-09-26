// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "husarnet/fstring.h"
#include "husarnet/string_view.h"

#include "enum.h"

#include <etl/string.h>

BETTER_ENUM(
    HookType,
    int,
    hosttable_changed = 1,
    whitelist_changed = 2,
    joined = 3,
    reconnected = 4,
    rw_request = 5,
    rw_release = 6)

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
T unpack(etl::string_view s)
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

inline std::string encodeHex(std::vector<unsigned char> s)
{
  std::string ret;
  for(unsigned char ch : s) {
    ret.push_back(hexLetters[(ch >> 4) & 0xF]);
    ret.push_back(hexLetters[ch & 0xF]);
  }
  return ret;
}

inline std::string encodeHex(std::string s)
{
  std::string ret;
  for(unsigned char ch : s) {
    ret.push_back(hexLetters[(ch >> 4) & 0xF]);
    ret.push_back(hexLetters[ch & 0xF]);
  }
  return ret;
}

inline std::string decodeHex(std::vector<unsigned char> s)
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

inline bool startsWith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(0, with.size()) == with;
}

inline bool endsWith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(s.size() - with.size()) == with;
}

inline std::string removePrefix(const std::string s, const std::string prefix)
{
  if(!startsWith(s, prefix)) {
    return s;
  }

  return s.substr(prefix.length());
}

std::vector<std::string> splitWhitespace(std::string s);
std::vector<std::string> split(std::string s, char byChar, int maxSplit);

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

template <typename K, typename V>
inline bool mapContains(const std::map<K, V> m, K needle)
{
  for(const auto& pair : m) {
    if(pair.first == needle) {
      return true;
    }
  }

  return false;
}

static inline const std::string
padRight(int minLength, const std::string& text, char paddingChar = ' ')
{
  int padSize = 0;

  if(text.length() < minLength) {
    padSize = minLength - text.length();
  }

  return text + std::string(padSize, paddingChar);
}

static inline const std::string
padLeft(int minLength, const std::string& text, char paddingChar = ' ')
{
  int padSize = 0;

  if(text.length() < minLength) {
    padSize = minLength - text.length();
  }

  return std::string(padSize, paddingChar) + text;
}

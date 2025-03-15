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

#include <etl/string.h>

#include "husarnet/fstring.h"
#include "husarnet/string_view.h"

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

std::string encodeHex(std::vector<unsigned char> s);
std::string encodeHex(std::string s);
std::string decodeHex(std::vector<unsigned char> s);
std::string decodeHex(std::string s);
std::string base64Decode(std::string s);

inline bool startsWith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(0, with.size()) == with;
}

inline bool endsWith(std::string s, std::string with)
{
  return s.size() >= with.size() && s.substr(s.size() - with.size()) == with;
}

std::string removePrefix(const std::string s, const std::string prefix);

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
std::string ltrim(std::string input);
std::string rtrim(std::string input);
std::string trim(std::string input);
std::string camelCaseToUnderscores(std::string camel);

bool strToBool(const std::string& s);

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

const std::string padRight(int minLength, const std::string& text, char paddingChar = ' ');
const std::string padLeft(int minLength, const std::string& text, char paddingChar = ' ');

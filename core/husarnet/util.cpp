// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/util.h"

#include <algorithm>
#include <climits>
#include <random>

#include <time.h>

std::vector<std::string> splitWhitespace(std::string s)
{
  std::vector<std::string> res;
  res.push_back("");
  for(char ch : s) {
    if(isspace(ch)) {
      if(res.back().size() > 0)
        res.push_back("");
    } else {
      res.back().push_back(ch);
    }
  }

  if(res.back().size() == 0)
    res.pop_back();

  return res;
}

std::vector<std::string> split(std::string s, char byChar, int maxSplit)
{
  std::vector<std::string> res;
  res.push_back("");
  for(char ch : s) {
    if(ch == byChar && res.size() <= maxSplit) {
      res.push_back("");
    } else {
      res.back().push_back(ch);
    }
  }

  return res;
}

std::string base64Decode(std::string s)
{
  std::string result;
  std::vector<int> charmap(256, -1);
  std::string characters(
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

  for(int i = 0; i < (int)characters.size(); i++)
    charmap[(uint8_t)characters[i]] = i;

  uint32_t val = 0;
  int pos = -8;
  for(uint8_t c : s) {
    if(charmap[c] == -1)
      continue;
    val = (val << 6) + charmap[c];
    pos += 6;
    if(pos >= 0) {
      result.push_back(char((val >> pos) & 0xFF));
      pos -= 8;
    }
  }

  return result;
}

static std::random_device rd;

std::string generateRandomString(const int length)
{
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> uni('a', 'z');

  std::string result = "";
  for(int i = 0; i < length; i++) {
    result += (char)uni(rng);
  }

  return result;
}

std::string strToUpper(std::string input)
{
  std::string ret(input);
  std::transform(input.begin(), input.end(), ret.begin(), [](unsigned char c) {
    return std::toupper(c);
  });
  return ret;
}

std::string strToLower(std::string input)
{
  std::string ret(input);
  std::transform(input.begin(), input.end(), ret.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return ret;
}

std::string rtrim(std::string s)
{
  s.erase(
      std::find_if(
          s.rbegin(), s.rend(),
          [](unsigned char ch) { return !std::isspace(ch); })
          .base(),
      s.end());
  return s;
}

std::string camelCaseToUnderscores(std::string camel)
{
  // TODO long term - add protection for strings too short
  std::string underscored = camel.substr(0, 1);
  for(auto it = camel.begin() + 1; it != camel.end(); it++) {
    if(isupper(*it) && islower(*(it - 1))) {
      underscored += "_";
    }

    underscored += *it;
  }

  return underscored;
}

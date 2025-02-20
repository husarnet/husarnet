// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/util.h"

#include <algorithm>
#include <climits>
#include <random>

#include <time.h>

std::string encodeHex(std::vector<unsigned char> s)
{
  std::string ret;
  for(unsigned char ch : s) {
    ret.push_back(hexLetters[(ch >> 4) & 0xF]);
    ret.push_back(hexLetters[ch & 0xF]);
  }
  return ret;
}

std::string encodeHex(std::string s)
{
  std::string ret;
  for(unsigned char ch : s) {
    ret.push_back(hexLetters[(ch >> 4) & 0xF]);
    ret.push_back(hexLetters[ch & 0xF]);
  }
  return ret;
}

std::string decodeHex(std::vector<unsigned char> s)
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

std::string decodeHex(std::string s)
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

std::string removePrefix(const std::string s, const std::string prefix)
{
  if(!startsWith(s, prefix)) {
    return s;
  }

  return s.substr(prefix.length());
}

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

std::string ltrim(std::string s)
{
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
  return s;
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

std::string trim(std::string s)
{
  return ltrim(rtrim(s));
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

bool strToBool(const std::string& s)
{
  auto r = trim(std::string(s));
  if(r == "true" || r == "1" || r == "yes" || r == "on") {
    return true;
  }

  return false;
}

const std::string
padRight(int minLength, const std::string& text, char paddingChar)
{
  int padSize = 0;

  if(text.length() < minLength) {
    padSize = minLength - text.length();
  }

  return text + std::string(padSize, paddingChar);
}

const std::string
padLeft(int minLength, const std::string& text, char paddingChar)
{
  int padSize = 0;

  if(text.length() < minLength) {
    padSize = minLength - text.length();
  }

  return std::string(padSize, paddingChar) + text;
}

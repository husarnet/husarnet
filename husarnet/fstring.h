// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <array>
#include <cassert>
#include <cstring>
#include <string>

template <int size>
struct fstring : std::array<unsigned char, size> {
  operator std::string() const
  {
    return std::string((char*)this->data(), this->size());
  }

  // cppcheck-suppress noExplicitConstructor
  fstring(std::string s)
  {
    assert(s.size() >= size);
    memcpy(this->data(), s.data(), size);
  }

  fstring()
  {
    memset(this->data(), 0, size);
  }

  std::string substr(int a, int l)
  {
    return std::string(*this).substr(a, l);
  }

  explicit fstring(const char* s)
  {
    memcpy(this->data(), s, size);
  }

  operator bool()
  {
    return *this == fstring();
  }
};

namespace std {
  template <>
  struct hash<fstring<16>> {
    unsigned operator()(const fstring<16>& a) const
    {
      unsigned s;
      memcpy(&s, &a[12], 4);
      return s;
    }
  };
}  // namespace std

template <int size, int size1>
std::string operator+(const fstring<size>& a, const fstring<size1>& b)
{
  return std::string(a) + std::string(b);
}

template <int size>
std::string operator+(const std::string& a, const fstring<size>& b)
{
  return a + std::string(b);
}

template <int size>
std::string& operator+=(std::string& a, const fstring<size>& b)
{
  return (a += std::string(b));
}

template <int size>
std::string operator+(const fstring<size>& a, const std::string& b)
{
  return std::string(a) + b;
}

template <int start, int size>
fstring<size> substr(const std::string& s)
{
  assert(s.size() >= start + size);
  return fstring<size>(s.substr(start, size));
}

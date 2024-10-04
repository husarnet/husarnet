// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <cassert>
#include <cstring>
#include <string>

#include <etl/string_view.h>

struct string_view {
  const char* m_data = nullptr;
  size_t m_size = 0;

  string_view()
  {
  }

  // cppcheck-suppress noExplicitConstructor
  string_view(const std::string& s)
  {
    m_data = s.data();
    m_size = s.size();
  }

  string_view(const etl::string_view& s)
  {
    m_data = s.data();
    m_size = s.size();
  }

  string_view(const char* data, size_t size) : m_data(data), m_size(size)
  {
  }

  string_view substr(long start, long size) const
  {
    assert(size >= 0);
    assert(start >= 0);
    assert(start <= (int)m_size);
    assert(start + size <= (int)m_size);
    return string_view(m_data + start, size);
  }

  string_view substr(long start) const
  {
    assert(start >= 0);
    assert(start <= (int)m_size);
    return string_view(m_data + start, m_size - start);
  }

  size_t size() const
  {
    return m_size;
  }

  const char* data() const
  {
    return m_data;
  }

  operator std::string() const
  {
    return std::string(m_data, m_size);
  }

  std::string str() const
  {
    return std::string(m_data, m_size);
  }

  const char& operator[](long index) const
  {
    assert(index >= 0 && index < (int)m_size);
    return m_data[index];
  }

  static string_view literal(const char* s)
  {
    return string_view(s, strlen(s));
  }
};

inline bool operator==(string_view a, string_view b)
{
  if(a.size() != b.size())
    return false;
  return memcmp(a.data(), b.data(), a.size()) == 0;
}

inline std::string& operator+=(std::string& s, string_view v)
{
  long prevLength = s.size();
  s.resize(s.size() + v.size());
  memcpy(&s[prevLength], v.data(), v.size());
  return s;
}

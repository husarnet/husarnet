// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <string>

class LogManager {
  class LogElement {
   public:
    std::string log;
    LogElement* next;
    LogElement* prev;
    explicit LogElement(std::string log)
        : log(log), next(nullptr), prev(nullptr){};
  };
  uint16_t size;
  uint16_t currentSize;
  LogElement* first;
  LogElement* last;
  uint16_t verbosity;

 public:
  explicit LogManager(uint16_t size)
      : size(size),
        currentSize(0),
        first(nullptr),
        last(nullptr),
        verbosity(3){};
  std::string getLogs();
  void setSize(uint16_t size);
  void insert(std::string log);
  void setVerbosity(uint16_t verb);
  uint16_t getVerbosity();
  uint16_t getSize();
  uint16_t getCurrentSize();
};

extern LogManager* globalLogManager;

// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/logmanager.h"

LogManager* globalLogManager;

std::string LogManager::getLogs()
{
  if(currentSize == 0)
    return "No logs to display";
  LogElement* itr = last;
  std::string result = "";
  while(itr != nullptr) {
    result += itr->log;
    result += '\n';
    itr = itr->prev;
  }
  return result;
};

void LogManager::insert(std::string& log)
{
  std::unique_lock<std::mutex> lock(mtx);
  prependLogTime(log);
  LogElement* logElement = new LogElement(log);
  if(last == nullptr or size == 1) {
    if(first != nullptr)
      delete first;
    currentSize = 1;
    first = logElement;
    last = logElement;
    return;
  }

  if(currentSize < size) {
    currentSize++;
    first->prev = logElement;
    logElement->next = first;
    first = logElement;
    return;
  }

  logElement->next = first;
  first->prev = logElement;
  first = logElement;
  last->prev->next = nullptr;
  LogElement* tmp = last;
  last = last->prev;
  delete tmp;
};

void LogManager::setSize(uint16_t size)
{
  std::unique_lock<std::mutex> lock(mtx);
  if(size >= currentSize) {
    this->size = size;
    return;
  }
  if(size < 1) {
    return;
  }
  if(size == 1) {
    LogElement* itr = first->next;
    first->next = nullptr;
    last = first;
    this->size = size;
    while(itr != nullptr) {
      LogElement* tmp = itr;
      itr = itr->next;
      delete tmp;
    }
    return;
  }

  LogElement* itr = first->next;
  LogElement* followItr = first;
  int visitedCount = 2;
  while(visitedCount < size && itr != nullptr) {
    followItr = itr;
    itr = itr->next;
    visitedCount++;
  }
  while(itr != nullptr) {
    LogElement* tmp = itr;
    itr = itr->next;
    delete tmp;
  }
  last = followItr;
  last->next = nullptr;
  this->size = size;
  this->currentSize = size;
};

void LogManager::setVerbosity(LogLevel verb)
{
  std::unique_lock<std::mutex> lock(mtx);
  verbosity = verb;
};

LogLevel LogManager::getVerbosity()
{
  return verbosity;
};

uint16_t LogManager::getSize()
{
  return size;
};

uint16_t LogManager::getCurrentSize()
{
  return currentSize;
};

void LogManager::prependLogTime(std::string& log)
{
  auto now = std::chrono::system_clock::now();
  auto now_c = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_c), "[%Y-%m-%d %H:%M:%S] ") << log;
  log = ss.str();
};

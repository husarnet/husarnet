// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/timer.h"
#include "husarnet/util.h"

class HooksManagerInterface {
 public:
  virtual ~HooksManagerInterface() = default;

  virtual void runHook(HookType hookType, bool immediate = false) = 0;
  virtual void waitHook(HookType hookType) = 0;
  virtual void cancelHook(HookType hookType) = 0;

  virtual void withRw(std::function<void()> f) = 0;
};

class HooksManager : public HooksManagerInterface {
 public:
  HooksManager(HusarnetManager* manager);
  ~HooksManager();

  virtual void runHook(HookType hookType, bool immediate = false);
  virtual void waitHook(HookType hookType);
  virtual void cancelHook(HookType hookType);

  virtual void withRw(std::function<void()> f);

 private:
  HusarnetManager* manager;

  // Basic hooks
  std::map<std::string, std::condition_variable*>
      hookConditionalVariables;  // dirName -> condition_variable
  std::mutex waitMutex;

  std::map<std::string, Timer*> hookTimers;  // dirName -> Timer

  // Rw logic
  bool isRw = false;
  std::mutex rwMutex;

  // Constants
  std::chrono::milliseconds timespan{
      1000};  // Time to postpone the hook execution on every runHook
  std::chrono::milliseconds interval{100};  // How often to re-check the timers
  std::chrono::milliseconds waitspan{
      5000};  // How long to wait for a script using waitHook

  std::map<HookType, std::string> hookDirNames{
      {HookType::hosttable_changed, "hook.hosttable_changed.d"},
      {HookType::whitelist_changed, "hook.whitelist_changed.d"},
      {HookType::joined, "hook.joined.d"},
      {HookType::reconnected, "hook.reconnected.d"},
      {HookType::rw_request, "hook.rw_request.d"},
      {HookType::rw_release, "hook.rw_release.d"},
  };
};

class DummyHooksManager : public HooksManagerInterface {
 public:
  virtual void runHook(HookType hookType, bool immediate = false)
  {
    LOG_DEBUG("DummyHooksManager::runHook %s", hookType._to_string());
  }

  virtual void waitHook(HookType hookType)
  {
    LOG_DEBUG("DummyHooksManager::waitHook %s", hookType._to_string());
  }

  virtual void cancelHook(HookType hookType)
  {
    LOG_DEBUG("DummyHooksManager::cancelHook %s", hookType._to_string());
  }

  virtual void withRw(std::function<void()> f)
  {
    LOG_DEBUG("DummyHooksManager::withRw enter");
    f();
    LOG_DEBUG("DummyHooksManager::withRw exit");
  }
};

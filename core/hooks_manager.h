// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>

#include "husarnet/ports/privileged_interface.h"

#include "husarnet/husarnet_manager.h"
#include "husarnet/logging.h"
#include "husarnet/timer.h"
#include "husarnet/util.h"

class HooksManagerInterface {
 public:
  virtual ~HooksManagerInterface() = default;

  virtual void runHook(HookType hookType) = 0;
  virtual void waitHook(HookType hookType) = 0;

  void withRw(std::function<void()> f)
  {
    this->runHook(HookType::rw_request);
    this->waitHook(HookType::rw_request);

    f();

    this->runHook(HookType::rw_release);
    this->waitHook(HookType::rw_release);
  }
};

class HooksManager : public HooksManagerInterface {
 public:
  HooksManager(HusarnetManager* manager);
  ~HooksManager();

  virtual void runHook(HookType hookType);
  virtual void waitHook(HookType hookType);

 private:
  HusarnetManager* manager;
  std::map<HookType, std::string> hookDirNames{
      {HookType::hosttable_changed, "hook.hosttable_changed.d"},
      {HookType::whitelist_changed, "hook.whitelist_changed.d"},
      {HookType::joinned, "hook.joined.d"},
      {HookType::reconnected, "hook.reconnected.d"},
      {HookType::rw_request, "hook.rw_request.d"},
      {HookType::rw_release, "hook.rw_release.d"}};
  std::map<std::string, Timer*> hookTimers;
  std::map<std::string, std::condition_variable*> hookConditionalVariables;
  std::mutex m;
  std::chrono::milliseconds timespan{500};
  std::chrono::milliseconds interval{10};
  std::chrono::milliseconds waitspan{5000};
};

class DummyHooksManager : public HooksManagerInterface {
 public:
  virtual void runHook(HookType hookType)
  {
    LOG_DEBUG("DummyHooksManager::runHook %s", hookType);
  }

  virtual void waitHook(HookType hookType)
  {
    LOG_DEBUG("DummyHooksManager::waitHook %s", hookType);
  }
};

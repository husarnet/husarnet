// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>

#include "husarnet/husarnet_manager.h"
#include "husarnet/timer.h"
#include "husarnet/util.h"

class HooksManager {
 public:
  HooksManager(HusarnetManager* manager)
  {
    this->manager = manager;
    for(const auto& [key_, value_] : hookDirNames) {
      hookConditionalVariables.insert({value_, new std::condition_variable()});
    }

    for(const auto& [key, value] : hookDirNames) {
      std::function<void()> callback = [&, val = value]() {
        Privileged::runScripts(val);
        std::lock_guard lk(this->m);
        hookConditionalVariables[val]->notify_all();
      };
      hookTimers.insert({value, new Timer(timespan, interval, callback)});
    }
  };

  ~HooksManager()
  {
    for(const auto& [key, value] : hookDirNames) {
      delete hookConditionalVariables[value];
      delete hookTimers[value];
    }
  }
  void runHook(HookType hookType);
  void waitHook(HookType hookType);

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
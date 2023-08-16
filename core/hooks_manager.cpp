// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/hooks_manager.h"

HooksManager::HooksManager(HusarnetManager* manager)
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

HooksManager::~HooksManager()
{
  for(const auto& [key, value] :  // cppcheck-suppress unusedVariable
      hookDirNames) {
    delete hookConditionalVariables[value];
    delete hookTimers[value];
  }
}

void HooksManager::runHook(HookType hookType)
{
  if(!Privileged::checkScriptsExist(hookDirNames[hookType])) {
    return;
  }

  hookTimers[hookDirNames[hookType]]->Reset();
}

void HooksManager::waitHook(HookType hookType)
{
  if(!manager->areHooksEnabled()) {
    return;
  }

  if(!Privileged::checkScriptsExist(hookDirNames[hookType])) {
    return;
  }

  std::unique_lock lk(m);
  hookConditionalVariables[hookDirNames[hookType]]->wait_for(lk, waitspan);
  lk.unlock();
}

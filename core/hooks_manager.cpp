// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/hooks_manager.h"

void HooksManager::runHook(HookType hookType)
{
  if(manager->areHooksEnabled()) {
    if(Privileged::checkScriptsExist(hookDirNames[hookType])) {
      hookTimers[hookDirNames[hookType]]->Reset();
    }
  }
}

void HooksManager::waitHook(HookType hookType)
{
  if(manager->areHooksEnabled()) {
    if(Privileged::checkScriptsExist(hookDirNames[hookType])) {
      std::unique_lock lk(m);
      hookConditionalVariables[hookDirNames[hookType]]->wait_for(lk, waitspan);
      lk.unlock();
    }
  }
}

void HooksManager::withRw(std::function<void()> f)
{
  this->runHook(HookType::rw_request);
  this->waitHook(HookType::rw_request);
  f();
  this->runHook(HookType::rw_release);
  this->waitHook(HookType::rw_release);
}

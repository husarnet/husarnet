// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/hooks_manager.h"

HooksManager::HooksManager(HusarnetManager* manager)
{
  this->manager = manager;

  for(const auto& [hookType_, dirName] : hookDirNames) {
    hookConditionalVariables.insert({dirName, new std::condition_variable()});
  }

  for(const auto& [hookType_ref, dirName_ref] : hookDirNames) {
    auto hookType = hookType_ref;
    auto dirName = dirName_ref;

    std::function<void()> callback = [this, hookType, dirName]() {
      if(!Privileged::checkScriptsExist(hookDirNames[hookType])) {
        return;
      }

      if(hookType == HookType::rw_request || hookType == HookType::rw_release) {
        rwMutex.lock();
      }

      Privileged::runScripts(dirName);

      if(hookType == HookType::rw_request) {
        isRw = true;
      }
      if(hookType == HookType::rw_release) {
        isRw = false;
      }

      if(hookType == HookType::rw_request || hookType == HookType::rw_release) {
        rwMutex.unlock();
      }

      std::lock_guard waitLock(this->waitMutex);
      hookConditionalVariables[dirName]->notify_all();
    };

    hookTimers.insert({dirName, new Timer(timespan, interval, callback)});
    hookTimers[dirName]->Start();
  }
};

HooksManager::~HooksManager()
{
  // cppcheck-suppress unusedVariable
  for(const auto& [hookType, dirName] : hookDirNames) {
    delete hookConditionalVariables[dirName];
    delete hookTimers[dirName];
  }
}

void HooksManager::runHook(HookType hookType, bool immediate)
{
  if(immediate) {
    // This one will execute immediately
    hookTimers[hookDirNames[hookType]]->Fire();
  } else {
    // This one will postpone execution
    hookTimers[hookDirNames[hookType]]->Reset();
  }
}

void HooksManager::waitHook(HookType hookType)
{
  std::unique_lock lk(waitMutex);
  hookConditionalVariables[hookDirNames[hookType]]->wait_for(
      lk, waitspan);  // This internally does an unlock, wait and lock again
  lk.unlock();
}

void HooksManager::cancelHook(HookType hookType)
{
  hookTimers[hookDirNames[hookType]]->Stop();
}

void HooksManager::withRw(std::function<void()> f)
{
  cancelHook(HookType::rw_release);

  rwMutex.lock();

  if(!isRw) {
    runHook(HookType::rw_request, true);
    rwMutex.unlock();
    waitHook(HookType::rw_request);
  } else {
    rwMutex.unlock();
  }

  f();

  runHook(HookType::rw_release);
}

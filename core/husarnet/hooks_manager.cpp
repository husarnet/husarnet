// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/hooks_manager.h"

#include "magic_enum/magic_enum_utility.hpp"

HooksManager::HooksManager(bool enableHooks)
{
  this->enabled = enableHooks;

  magic_enum::enum_for_each<HookType>([this](auto val) {
    constexpr HookType hook = val;
    this->hookTimers[hook] = -1;
  });
};

void HooksManager::scheduleHook(const HookType hookType)
{
  if(!this->enabled) {
    return;
  }

  this->mutex.lock();
  this->hookTimers[hookType] = HOOKS_BUMP_TIME;
  this->mutex.unlock();
}

void HooksManager::periodicThread()
{
  if(!this->enabled) {
    return;
  }

  while(true) {
    this->mutex.lock();
    magic_enum::enum_for_each<HookType>([this](auto val) {
      constexpr HookType hook = val;

      if(this->hookTimers[hook] >= 0) {
        this->hookTimers[hook] -= HOOKS_PERIOD;
        if(this->hookTimers[hook] <= 0) {
          Port::runHook(hook);
          this->hookTimers[hook] = -1;
        }
      }
    });

    this->mutex.unlock();

    Port::threadSleep(HOOKS_PERIOD);
  }
}

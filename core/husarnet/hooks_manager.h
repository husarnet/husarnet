// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include "husarnet/config_env.h"
#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "etl/map.h"
#include "etl/mutex.h"

#ifndef HOOKS_MAP_SIZE
#define HOOKS_MAP_SIZE 10  // Keep this in sync with size of HookType enum
#endif

#ifndef HOOKS_PERIOD
#define HOOKS_PERIOD 100  // ms, processing period
#endif

#ifndef HOOKS_BUMP_TIME
#define HOOKS_BUMP_TIME \
  200  // ms, each schedule will set hook's timer to this amount of time. Hook
       // will be executed after it times out
#endif

class HooksManager {
 private:
  bool enabled;

  etl::mutex mutex;
  etl::map<HookType, int, HOOKS_MAP_SIZE>
      hookTimers;  // >0 time in ms. 0 means fire now, <0 means not set/already
                   // fired

 public:
  HooksManager(bool enableHooks);

  void periodicThread();  // Start this as a thread - will handle timers and
                          // actually calling the hooks
  void scheduleHook(const HookType hookType);  // Threadsafe
};

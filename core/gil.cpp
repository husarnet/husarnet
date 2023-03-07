// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/gil.h"

#include <condition_variable>
#include <mutex>
#include <thread>

#include "husarnet/ports/port.h"
#include "husarnet/ports/port_interface.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

namespace GIL {

  std::mutex globalLock;

  void init()
  {
    lock();
  }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack,
      int priority)
  {
    auto wrapped = [func]() {
      globalLock.lock();
      func();
      globalLock.unlock();
    };
    Port::startThread(wrapped, name, stack, priority);
  }

  void yield()
  {
    globalLock.unlock();
    // std::this_thread::yield();
    globalLock.lock();
  }

#ifndef DEBUG_LOCKS
  void lock()
  {
    globalLock.lock();
  }

  void unlock()
  {
    globalLock.unlock();
  }
#else
  void lock()
  {
    if(globalLock.try_lock()) {
      LOG_DEBUG("lock");
    } else {
      LOG_DEBUG("wait for lock");
      globalLock.lock();
      LOG_DEBUG("locked");
    }
  }

  void unlock()
  {
    globalLock.unlock();
    LOG_DEBUG("unlock");
  }
#endif

}  // namespace GIL

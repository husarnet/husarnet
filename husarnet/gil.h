// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <functional>

namespace GIL {
  void lock();
  void unlock();

  template <typename T, typename Functor>
  struct unlocked_impl {
    static T run(Functor f)
    {
      unlock();
      T val = f();
      lock();
      return val;
    }
  };

  template <typename Functor>
  struct unlocked_impl<void, Functor> {
    static void run(Functor f)
    {
      unlock();
      f();
      lock();
    }
  };

  template <typename T, typename Functor>
  T unlocked(Functor f)
  {
    return unlocked_impl<T, Functor>::run(f);
  }

  void startThread(
      std::function<void()> func,
      const char* name,
      int stack = -1,
      int priority = 2);
  void yield();
  void init();
}  // namespace GIL

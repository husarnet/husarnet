// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

class PeriodicTimer {
 public:
  PeriodicTimer(
      std::chrono::milliseconds interval,
      std::function<void()> callback)
      : interval(interval), callback(callback)
  {
  }

  ~PeriodicTimer()
  {
    this->Stop();
  }

  void Start()
  {
    running = true;
    thread = std::thread([this] {
      while(running) {
        std::unique_lock lk(m);
        convar.wait_for(lk, interval);
        lk.unlock();
        if(running)
          callback();
      }
    });
  }

  void Stop()
  {
    running = false;
    std::unique_lock lk(m);
    convar.notify_one();
    lk.unlock();
    if(thread.joinable()) {
      thread.join();
    }
  }

  void Reset()
  {
    Stop();
    Start();
  }

 private:
  std::chrono::milliseconds interval;
  std::function<void()> callback;
  std::thread thread;
  std::atomic<bool> running;
  std::mutex m;
  std::condition_variable convar;
};

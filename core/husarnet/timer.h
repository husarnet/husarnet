// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

class Timer {
 public:
  // Interval is the time between evaluations of the call conditions
  // Timespan is the time that each Reset call uses to postpone the execution
  // Callback willl be called **in the timer's thread** after the timer expires
  Timer(
      std::chrono::milliseconds interval,
      std::chrono::milliseconds timespan,
      std::function<void()> callback)
      : interval(interval), timespan(timespan), callback(callback)
  {
    expiration_time = std::chrono::time_point<std::chrono::steady_clock>::max();
  }

  ~Timer()
  {
    Stop();
  }

  // Start the timer in the background
  void Start()
  {
    thread = std::thread([this] {
      while(true) {
        std::this_thread::sleep_for(interval);

        mutex.lock();

        bool should_run = false;
        auto now = std::chrono::steady_clock::now();
        if(now >= expiration_time) {
          should_run = true;

          expiration_time =
              std::chrono::time_point<std::chrono::steady_clock>::max();
        }
        mutex.unlock();

        if(should_run) {
          callback();
        }
      }
    });
  }

  // Stop the timer and prevent it from execution
  // Also - cleanup the thread
  void Stop()
  {
    std::lock_guard waitLock(mutex);
    expiration_time = std::chrono::time_point<std::chrono::steady_clock>::max();
  }

  // Reset the timer, postponing the execution to at least timespan from now
  void Reset()
  {
    std::lock_guard waitLock(mutex);
    expiration_time = std::chrono::steady_clock::now() + timespan;
  }

  // Fire the callback immediately
  void Fire()
  {
    std::lock_guard waitLock(mutex);
    expiration_time =
        std::chrono::steady_clock::now() - std::chrono::seconds(1);
  }

 private:
  std::chrono::milliseconds interval;
  std::chrono::milliseconds timespan;
  std::function<void()> callback;
  std::thread thread;
  std::chrono::time_point<std::chrono::steady_clock> expiration_time;
  std::mutex mutex;
};

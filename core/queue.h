// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

// simple thread safe queue

template <typename T>
struct Queue {
  std::mutex m;
  std::condition_variable cv;
  std::deque<T> q;

  int qsize()
  {
    std::unique_lock<std::mutex> g(m);
    return q.size();
  }

  void push(T&& v)
  {
    std::unique_lock<std::mutex> g(m);
    q.push_back(std::move(v));
    cv.notify_one();
  }

  T pop_blocking()
  {
    std::unique_lock<std::mutex> g(m);
    cv.wait(g, [this]() { return q.size() > 0; });
    T out = std::move(q.front());
    q.pop_front();
    return out;
  }
};

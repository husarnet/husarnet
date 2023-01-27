// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#pragma once
#include <thread>
#include <functional>
#include <atomic>
#include <chrono>
class Timer {
public:
    Timer(std::chrono::milliseconds interval,std::chrono::milliseconds timespan, std::function<void()> callback)
        : interval(interval), timespan(timespan), callback(callback) {}

    ~Timer(){
        this->Stop();
    }
    
    void Start() {
        running = true;
        expiration_time = std::chrono::steady_clock::now() + timespan;
        thread = std::thread([this] {
            while (running) {
                auto now = std::chrono::steady_clock::now();
                if (now >= expiration_time) {
                    callback();
                    running = false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
        });
    }

    void Stop() {
        running = false;
        if (thread.joinable()) {
            thread.join();
        }
    }

    void Reset() {
        Stop();
        expiration_time = std::chrono::steady_clock::now() + timespan;
        Start();
    }

private:
    std::chrono::milliseconds interval;
    std::chrono::milliseconds timespan;
    std::function<void()> callback;
    std::thread thread;
    std::atomic<bool> running;
    std::chrono::time_point<std::chrono::steady_clock> expiration_time;
};
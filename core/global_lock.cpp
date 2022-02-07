#include "global_lock.h"
#include "util.h"
namespace GIL {

std::mutex globalLock;

void init() {
  lock();
}

void startThread(std::function<void()> func,
                 const char* name,
                 int stack,
                 int priority) {
  auto wrapped = [func]() {
    globalLock.lock();
    func();
    globalLock.unlock();
  };
  ::startThread(wrapped, name, stack, priority);
}

void yield() {
  globalLock.unlock();
  // std::this_thread::yield();
  globalLock.lock();
}

#ifndef DEBUG_LOCKS
void lock() {
  globalLock.lock();
}

void unlock() {
  globalLock.unlock();
}
#else
void lock() {
  if (globalLock.try_lock()) {
    LOG("lock");
  } else {
    LOG("wait for lock");
    globalLock.lock();
    LOG("locked");
  }
}

void unlock() {
  globalLock.unlock();
  LOG("unlock");
}
#endif

}  // namespace GIL

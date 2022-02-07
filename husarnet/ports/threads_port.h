#pragma once
#if defined(_GLIBCXX_HAS_GTHREADS)
#include <condition_variable>
#include <mutex>
#include <thread>
#else
#include "mingw.condition_variable.h"
#include "mingw.future.h"
#include "mingw.mutex.h"
#include "mingw.shared_mutex.h"
#include "mingw.thread.h"
#endif

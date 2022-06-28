// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
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

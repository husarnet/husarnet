#if defined(_GLIBCXX_HAS_GTHREADS)
#include <mutex>
#include <condition_variable>
#else
#include "mingw.condition_variable.h"
#include "mingw.future.h"
#include "mingw.mutex.h"
#include "mingw.shared_mutex.h"
#include "mingw.thread.h"
#endif

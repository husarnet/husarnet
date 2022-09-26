set(CMAKE_SYSTEM_NAME Win)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(TOOLCHAIN_PREFIX /usr/bin/i686-w64-mingw32)
set(TOOLCHAIN_FLAGS "")

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TOOLCHAIN_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TOOLCHAIN_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TOOLCHAIN_FLAGS}")

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
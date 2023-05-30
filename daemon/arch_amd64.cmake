set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(triple x86_64-linux-musl)
set(zig_extra -march=x86_64_v3 -mtune=x86_64_v3)

set(CMAKE_C_COMPILER "zig" cc -target ${triple} ${zig_extra})
set(CMAKE_CXX_COMPILER "zig" c++ -target ${triple} ${zig_extra})

set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig-ar.sh")
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig-ranlib.sh")

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

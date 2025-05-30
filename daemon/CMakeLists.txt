cmake_minimum_required(VERSION 3.22.0)
cmake_policy(SET CMP0003 NEW)

set(CMAKE_C_COMPILER ${CMAKE_C_COMPILER} ${COMPILER_COMMON})
set(CMAKE_CXX_COMPILER ${CMAKE_CXX_COMPILER} ${COMPILER_COMMON})

file(READ ../version.txt husarnet_version)
string(STRIP ${husarnet_version} husarnet_version)
project(husarnet VERSION ${husarnet_version})

include(../core/husarnet.cmake)

file(GLOB husarnet_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_executable(husarnet-daemon ${husarnet_SRC})
target_link_libraries(husarnet-daemon -Wl,--whole-archive husarnet_core -Wl,--no-whole-archive)

if(CMAKE_SYSTEM_NAME EQUAL Windows)
    install(TARGETS husarnet-daemon)
else()
    install(TARGETS husarnet-daemon)
endif()

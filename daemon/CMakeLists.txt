cmake_minimum_required(VERSION 3.14.0)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

file(READ ../version.txt husarnet_version)
string(STRIP ${husarnet_version} husarnet_version)
project(husarnet VERSION ${husarnet_version})

include(../core/husarnet.cmake)

file(GLOB husarnet_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_executable(husarnet-daemon ${husarnet_SRC})
target_link_libraries(husarnet-daemon husarnet_core)

if(CMAKE_SYSTEM_NAME EQUAL Windows)
    install(TARGETS husarnet-daemon)
else()
    install(TARGETS husarnet-daemon)
endif()

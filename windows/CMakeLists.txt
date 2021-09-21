cmake_minimum_required (VERSION 2.4)

project (husarnet)
cmake_policy(SET CMP0003 NEW)

include(../husarnet.cmake)

file(GLOB husarnet_SRC "*.cpp")
add_executable(husarnet ${husarnet_SRC})
target_link_libraries(husarnet husarnet_core)

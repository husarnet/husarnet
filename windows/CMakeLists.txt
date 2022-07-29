cmake_minimum_required (VERSION 3.5.0)
cmake_policy(SET CMP0003 NEW)

project (husarnet VERSION "0.0.0")

include(../husarnet/husarnet.cmake)

file(GLOB husarnet_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_executable(husarnet-daemon ${husarnet_SRC})
target_link_libraries(husarnet-daemon husarnet_core)

install(TARGETS husarnet-daemon DESTINATION bin)

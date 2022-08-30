cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(husarnet VERSION "0.0.0")

include(../../core/husarnet.cmake)

file(GLOB husarnet_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
add_executable(husarnet-daemon ${husarnet_SRC})
target_link_libraries(husarnet-daemon husarnet_core)

install(TARGETS husarnet-daemon DESTINATION usr/bin)
install(FILES
    ../../platforms/unix/packaging-misc/husarnet.service
    DESTINATION lib/systemd/system)
install(FILES
    ../../platforms/unix/packaging-misc/autocomplete
    RENAME husarnet
    DESTINATION usr/share/bash-completion/completions)

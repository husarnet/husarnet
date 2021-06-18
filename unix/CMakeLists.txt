cmake_minimum_required (VERSION 3.5.0)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project (husarnet VERSION "0.0.0")

include(../husarnet.cmake)
include(../tests/tests.cmake)

file(GLOB husarnet_SRC "*.cpp")
add_executable(husarnet ${husarnet_SRC})
target_link_libraries(husarnet -static husarnet_core)
install(TARGETS husarnet DESTINATION usr/bin)
install(FILES
    packaging-misc/husarnet.service
    packaging-misc/husarnet-l2.service
    packaging-misc/husarnet-license-refresh.service
    packaging-misc/husarnet-license-refresh.timer
    DESTINATION lib/systemd/system)

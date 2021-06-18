set(D ${CMAKE_CURRENT_LIST_DIR})
set(catch_path ${CMAKE_CURRENT_LIST_DIR}/../deps/Catch2)

file(GLOB test_SRC "${D}/*.cpp" "${D}/common/*.cpp" "${D}/core/*.cpp")

add_executable(tests ${test_SRC} ${catch_SRC})
target_link_libraries(tests husarnet_core)
include_directories(${catch_path}/single_include/catch2)

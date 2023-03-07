cmake_minimum_required(VERSION 3.16)

project(husarnet_esp32 C)

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

idf_build_process(esp32)

enable_language(C)
enable_language(CXX)

set(CMAKE_SYSTEM_PROCESSOR "${TARGET}")
set(CMAKE_SYSTEM_NAME "ESP32")

include(../../core/husarnet.cmake)

target_compile_options(husarnet_core PRIVATE -DESP_PLATFORM=1)

target_link_libraries(husarnet_core idf::newlib)
target_link_libraries(husarnet_core idf::lwip)
target_link_libraries(husarnet_core idf::pthread)
target_link_libraries(husarnet_core idf::nvs_flash)

add_executable(${CMAKE_PROJECT_NAME}.elf main.c)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf husarnet_core)
idf_build_executable(${CMAKE_PROJECT_NAME}.elf)

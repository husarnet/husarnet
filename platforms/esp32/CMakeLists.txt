cmake_minimum_required(VERSION 3.16)

#include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
#include($ENV{IDF_PATH}/tools/cmake/toolchain-clang-${TARGET}.cmake)

enable_language(C)
enable_language(CXX)

message(STATUS ${CMAKE_SOURCE_DIR})

set(IDF_TARGET "${TARGET}")
set(IDF_TOOLCHAIN clang)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(husarnet_esp32_core)
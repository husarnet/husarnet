cmake_minimum_required(VERSION 3.16)

enable_language(C)
enable_language(CXX)

set(CMAKE_SYSTEM_PROCESSOR "${TARGET}")
#set(IDF_TOOLCHAIN clang)

project(husarnet_esp32_core)

include(${CMAKE_SOURCE_DIR}/../../core/husarnet.cmake)

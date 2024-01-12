cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_PROCESSOR "${TARGET}")
#set(IDF_TOOLCHAIN clang)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(husarnet_esp32_core)

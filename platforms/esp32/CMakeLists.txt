cmake_minimum_required(VERSION 3.16)

set(CMAKE_SYSTEM_PROCESSOR "${TARGET}")

set(EXTRA_COMPONENT_DIRS 
  $ENV{IDF_PATH}/examples/common_components/iperf
  $ENV{IDF_PATH}/tools/unit-test-app/components
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(husarnet_esp32_core)

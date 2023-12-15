# Build husarnet core
set(CMAKE_SYSTEM_PROCESSOR "${TARGET}")
set(CMAKE_SYSTEM_NAME "ESP32")

include(${CMAKE_SOURCE_DIR}/../../core/husarnet.cmake)

message(STATUS "idf_component_register core")
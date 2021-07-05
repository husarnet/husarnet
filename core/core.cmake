set(D ${CMAKE_CURRENT_LIST_DIR})

include_directories(${D}/../deps/ArduinoJson)
include_directories(${D})
file(GLOB husarnet_core_SRC "${D}/*.cpp")
add_library(husarnet_core STATIC ${husarnet_core_SRC})
target_link_libraries(husarnet_core husarnet_common sodium zstd sqlite3)


include(FetchContent)

FetchContent_Declare(
  httplib
  SOURCE_DIR "${D}/../deps/cpp-httplib"
)

FetchContent_GetProperties(httplib)
if(NOT httplib_POPULATED)
  FetchContent_Populate(httplib)
  set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
  set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
  add_subdirectory(${httplib_SOURCE_DIR} ${httplib_BINARY_DIR} EXCLUDE_FROM_ALL)
  target_link_libraries(husarnet_core httplib)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(husarnet_core log)
endif()

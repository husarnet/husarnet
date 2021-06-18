set(D ${CMAKE_CURRENT_LIST_DIR})

include_directories(${D}/../deps/ArduinoJson)
include_directories(${D})
file(GLOB husarnet_core_SRC "${D}/*.cpp")
add_library(husarnet_core STATIC ${husarnet_core_SRC})
target_link_libraries(husarnet_core husarnet_common sodium zstd sqlite3)

if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(husarnet_core log)
endif()

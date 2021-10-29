## Compile flags

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (DEBUG)
  set(COMMONFLAGS "${COMMONFLAGS} -D_GLIBCXX_DEBUG -g -fsanitize=undefined -fsanitize=undefined") # -fsanitize=thread
else()
  set(COMMONFLAGS "${COMMONFLAGS} -O3 -ffunction-sections -fdata-sections")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(COMMONFLAGS "${COMMONFLAGS} -fPIC -fstack-protector-strong")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
  set(COMMONFLAGS "${COMMONFLAGS} -fPIC -fstack-protector-strong")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(COMMONFLAGS "${COMMONFLAGS} -mconsole -mthreads -DWindows")
  set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -Wall -Werror=return-type -Wno-sign-compare ${COMMONFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${COMMONFLAGS}")

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,-z,relro -Wl,-z,now -Wl,--gc-sections")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,--gc-sections")
endif()

if (DEFINED FAIL_ON_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

## Commons library

set(D ${CMAKE_CURRENT_LIST_DIR})
include_directories(${D})

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include_directories(${D}/unix)
  file(GLOB husarnet_common_port_SRC "${D}/unix/*.cpp")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  include_directories(${D}/windows)
  file(GLOB husarnet_common_port_SRC "${D}/windows/*.cpp")
endif()

file(GLOB husarnet_common_SRC "${D}/*.cpp")
add_library(husarnet_common STATIC ${husarnet_common_SRC} ${husarnet_common_port_SRC})

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include(FetchContent)

  FetchContent_Declare(
    c-ares
    SOURCE_DIR "${D}/../../deps/c-ares"
  )

  FetchContent_GetProperties(c-ares)
  if(NOT c-ares_POPULATED)
    FetchContent_Populate(c-ares)
    set(CMAKE_PROJECT_VERSION 0.0.0)
    set(CARES_SHARED OFF)
    set(CARES_STATIC ON)
    set(CARES_STATIC_PIC ON)
    add_subdirectory(${c-ares_SOURCE_DIR} ${c-ares_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_common c-ares)
  endif()

endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  target_link_libraries(husarnet_common -Wl,--whole-archive -lrt -lpthread -Wl,--no-whole-archive)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  include_directories(${D}/../../deps/mingw-std-threads/)
  target_link_libraries(husarnet_common iphlpapi shlwapi ws2_32)
endif()

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=return-type -Wno-sign-compare ${COMMONFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${COMMONFLAGS}")

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,-z,relro -Wl,-z,now -Wl,--gc-sections -Wl,--whole-archive -lrt -lpthread -Wl,--no-whole-archive")
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,--gc-sections")
endif()

if (DEFINED FAIL_ON_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

# Add all required headers and source files
list(APPEND husarnet_core_SRC)  # This is more of a define rather than an append

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/unix)
  file(GLOB port_unix_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/unix/*.cpp")
  list(APPEND husarnet_core_SRC ${port_unix_SRC})
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/windows)
  file(GLOB port_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_windows_SRC})
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/ports)
file(GLOB husarnet_ports_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/*.cpp")
list(APPEND husarnet_core_SRC ${husarnet_ports_SRC})

include_directories(${CMAKE_CURRENT_LIST_DIR})
file(GLOB core_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
list(APPEND husarnet_core_SRC ${core_SRC})

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR (${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  include_directories(${CMAKE_CURRENT_LIST_DIR}/cli)
  file(GLOB cli_SRC "${CMAKE_CURRENT_LIST_DIR}/cli/*.cpp")
  list(APPEND husarnet_core_SRC ${cli_SRC})

  include_directories(${CMAKE_CURRENT_LIST_DIR}/api_server)
  file(GLOB api_server_SRC "${CMAKE_CURRENT_LIST_DIR}/api_server/*.cpp")
  list(APPEND husarnet_core_SRC ${api_server_SRC})
endif()

add_library(husarnet_core STATIC ${husarnet_core_SRC})

# Configure dependencies
include(FetchContent)

include(${CMAKE_CURRENT_LIST_DIR}/../deps/deps.cmake)
target_link_libraries(husarnet_core sodium zstd sqlite3)

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  target_link_libraries(husarnet_core iphlpapi shlwapi ws2_32)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(husarnet_core log)
endif()

# Include unix port libraries
if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  FetchContent_Declare(
    c-ares
    GIT_REPOSITORY https://github.com/c-ares/c-ares.git
    GIT_TAG        cares-1_18_1
  )

  FetchContent_GetProperties(c-ares)
  if(NOT c-ares_POPULATED)
    FetchContent_Populate(c-ares)
    set(CMAKE_PROJECT_VERSION 0.0.0)
    set(CARES_SHARED OFF)
    set(CARES_STATIC ON)
    set(CARES_STATIC_PIC ON)
    add_subdirectory(${c-ares_SOURCE_DIR} ${c-ares_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_core c-ares)
  endif()
endif()

# Include windows port libraries
if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  FetchContent_Declare(
    mingw-std-threads
    GIT_REPOSITORY https://github.com/meganz/mingw-std-threads.git
    GIT_TAG        f73afbe664bf3beb93a01274246de80d543adf6e
  )

  FetchContent_GetProperties(mingw-std-threads)
  if(NOT mingw-std-threads_POPULATED)
    FetchContent_Populate(mingw-std-threads)
    option(MINGW_STDTHREADS_GENERATE_STDHEADERS "" ON)
    add_subdirectory(${mingw-std-threads_SOURCE_DIR} ${mingw-std-threads_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_core mingw_stdthreads)
  endif()
endif()

# Enable HTTP control API and CLI
if (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR (${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  add_compile_definitions(HTTP_CONTROL_API)

  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.10.1
  )

  FetchContent_GetProperties(httplib)
  if(NOT httplib_POPULATED)
    FetchContent_Populate(httplib)
    set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
    add_subdirectory(${httplib_SOURCE_DIR} ${httplib_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_core httplib)
    # This is a hack working only for some specific libraries. Do not copy it to other places
    include_directories(${httplib_SOURCE_DIR})
  endif()

  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.10.5
  )

  FetchContent_GetProperties(nlohmann_json)
  if(NOT nlohmann_json_POPULATED)
    FetchContent_Populate(nlohmann_json)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_core nlohmann_json)
  endif()

  FetchContent_Declare(
    docopt
    GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
    GIT_TAG        v0.6.3
  )

  FetchContent_GetProperties(docopt)
  if(NOT docopt_POPULATED)
    FetchContent_Populate(docopt)
    add_subdirectory(${docopt_SOURCE_DIR} ${docopt_BINARY_DIR} EXCLUDE_FROM_ALL)
    target_link_libraries(husarnet_core docopt_s)
  endif()
endif()

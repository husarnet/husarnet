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

# Configure the build
set(BUILD_HTTP_CONTROL_API FALSE)

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux OR (${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  set(BUILD_HTTP_CONTROL_API TRUE)
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

include_directories(${CMAKE_CURRENT_LIST_DIR}/privileged)
file(GLOB husarnet_privileged_SRC "${CMAKE_CURRENT_LIST_DIR}/privileged/*.cpp")
list(APPEND husarnet_core_SRC ${husarnet_privileged_SRC})

if (${BUILD_HTTP_CONTROL_API})
  include_directories(${CMAKE_CURRENT_LIST_DIR}/cli)
  file(GLOB cli_SRC "${CMAKE_CURRENT_LIST_DIR}/cli/*.cpp")
  list(APPEND husarnet_core_SRC ${cli_SRC})

  include_directories(${CMAKE_CURRENT_LIST_DIR}/api_server)
  file(GLOB api_server_SRC "${CMAKE_CURRENT_LIST_DIR}/api_server/*.cpp")
  list(APPEND husarnet_core_SRC ${api_server_SRC})
endif()

# Top level project files

# @TODO make this work
# So includes like "husarnet/something.h" do work too
# include_directories(${CMAKE_CURRENT_LIST_DIR}/..)
include_directories(${CMAKE_CURRENT_LIST_DIR})

file(GLOB core_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
list(APPEND husarnet_core_SRC ${core_SRC})

# Join all of the above
add_library(husarnet_core STATIC ${husarnet_core_SRC})

# Configure dependencies
include(FetchContent)

include(${CMAKE_CURRENT_LIST_DIR}/../deps/deps.cmake)
target_link_libraries(husarnet_core sodium sqlite3)

if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  target_link_libraries(husarnet_core iphlpapi shlwapi ws2_32)
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(husarnet_core log)
endif()

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.10.5
)
set(JSON_BuildTests OFF)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(husarnet_core nlohmann_json)

FetchContent_Declare(
  better_enums
  GIT_REPOSITORY https://github.com/aantron/better-enums.git
  GIT_TAG        0.11.3
)
FetchContent_MakeAvailable(better_enums)
# target_link_libraries(husarnet_core better_enums)
target_include_directories(husarnet_core PUBLIC ${better_enums_SOURCE_DIR})

# Include unix port libraries
if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  FetchContent_Declare(
    c-ares
    GIT_REPOSITORY https://github.com/c-ares/c-ares.git
    GIT_TAG        cares-1_18_1
  )
  set(CARES_SHARED OFF)
  set(CARES_STATIC ON)
  set(CARES_STATIC_PIC ON)
  set(CARES_INSTALL OFF)
  set(CARES_BUILD_TESTS OFF)
  set(CARES_BUILD_CONTAINER_TESTS OFF)
  set(CARES_BUILD_TOOLS OFF)
  add_compile_definitions(CARES_STATICLIB)
  FetchContent_MakeAvailable(c-ares)
  target_link_libraries(husarnet_core c-ares)
endif()

# Include windows port libraries
if (${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  FetchContent_Declare(
    mingw-std-threads
    GIT_REPOSITORY https://github.com/meganz/mingw-std-threads.git
    GIT_TAG        f73afbe664bf3beb93a01274246de80d543adf6e
  )
  set(MINGW_STDTHREADS_GENERATE_STDHEADERS ON)
  FetchContent_MakeAvailable(mingw-std-threads)
  target_link_libraries(husarnet_core mingw_stdthreads)
endif()

# Enable HTTP control API and CLI
if (${BUILD_HTTP_CONTROL_API})
  add_compile_definitions(HTTP_CONTROL_API)

  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.10.1
  )
  set(BUILD_SHARED_LIBS OFF)
  set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
  set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
  set(HTTPLIB_REQUIRE_OPENSSL OFF)
  set(HTTPLIB_REQUIRE_ZLIB OFF)
  set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
  set(HTTPLIB_REQUIRE_BROTLI OFF)
  set(HTTPLIB_COMPILE ON)
  FetchContent_MakeAvailable(httplib)
  target_link_libraries(husarnet_core httplib)
  # include_directories(${httplib_SOURCE_DIR})

  if(IS_DIRECTORY "${httplib_SOURCE_DIR}")
    set_property(DIRECTORY ${httplib_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
  endif()

  FetchContent_Declare(
    docopt
    GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
    GIT_TAG        v0.6.3
  )
  FetchContent_MakeAvailable(docopt)
  target_link_libraries(husarnet_core docopt_s)

  if(IS_DIRECTORY "${docopt_SOURCE_DIR}")
    set_property(DIRECTORY ${docopt_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
  endif()
endif()
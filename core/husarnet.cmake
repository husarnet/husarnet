set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(COMMONFLAGS "${COMMONFLAGS} -D_GLIBCXX_DEBUG -g -fsanitize=undefined -fsanitize=undefined") # -fsanitize=thread

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
else()
  set(COMMONFLAGS "${COMMONFLAGS} -O3 -ffunction-sections -fdata-sections")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(COMMONFLAGS "${COMMONFLAGS} -fPIC -fstack-protector-strong")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Android)
  set(COMMONFLAGS "${COMMONFLAGS} -fPIC -fstack-protector-strong")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(COMMONFLAGS "${COMMONFLAGS} -DWindows")
  set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libgcc -static-libstdc++ -lwsock32 -lws2_32 ${CMAKE_CXX_STANDARD_LIBRARIES}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=return-type -Wno-sign-compare ${COMMONFLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${COMMONFLAGS}")

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -lrt -lpthread")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,--gc-sections")
endif()

if(DEFINED FAIL_ON_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

# Configure the build
set(BUILD_HTTP_CONTROL_API FALSE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  set(BUILD_HTTP_CONTROL_API TRUE)
endif()

# Add all required headers and source files
list(APPEND husarnet_core_SRC) # This is more of a define rather than an append

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/unix)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_unix_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/unix/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_unix_SRC} ${port_shared_unix_windows_SRC})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/windows)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/windows/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_windows_SRC} ${port_shared_unix_windows_SRC})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/macos)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_macos_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/macos/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_macos_SRC} ${port_shared_unix_windows_SRC})
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/ports)
file(GLOB husarnet_ports_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/*.cpp")
list(APPEND husarnet_core_SRC ${husarnet_ports_SRC})

include_directories(${CMAKE_CURRENT_LIST_DIR}/privileged)
file(GLOB husarnet_privileged_SRC "${CMAKE_CURRENT_LIST_DIR}/privileged/*.cpp")
list(APPEND husarnet_core_SRC ${husarnet_privileged_SRC})

if(${BUILD_HTTP_CONTROL_API})
  include_directories(${CMAKE_CURRENT_LIST_DIR}/api_server)
  file(GLOB api_server_SRC "${CMAKE_CURRENT_LIST_DIR}/api_server/*.cpp")
  list(APPEND husarnet_core_SRC ${api_server_SRC})
endif()

# Top level project files
include_directories(${CMAKE_CURRENT_LIST_DIR})

file(GLOB core_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
list(APPEND husarnet_core_SRC ${core_SRC})

# "Aliasing" (copying) core folder as "husarnet"
# So includes like "husarnet/something.h" do work too
set(TEMP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/tempIncludes)
file(MAKE_DIRECTORY ${TEMP_INCLUDE_DIR})
file(CREATE_LINK ${CMAKE_CURRENT_LIST_DIR}/ ${TEMP_INCLUDE_DIR}/husarnet SYMBOLIC)

# Join all of the above
add_library(husarnet_core STATIC ${husarnet_core_SRC})
target_include_directories(husarnet_core PUBLIC ${TEMP_INCLUDE_DIR})
target_compile_definitions(husarnet_core PRIVATE PORT_ARCH="${CMAKE_SYSTEM_PROCESSOR}")

# Configure dependencies
include(FetchContent)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  target_link_libraries(husarnet_core iphlpapi shlwapi ws2_32)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(husarnet_core log)
endif()

FetchContent_Declare(
  libsodium
  GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
  GIT_TAG 1.0.14
)
FetchContent_MakeAvailable(libsodium)
include_directories(${libsodium_SOURCE_DIR}/src/libsodium/include)
file(GLOB_RECURSE sodium_SRC ${libsodium_SOURCE_DIR}/src/libsodium/*.c)
add_library(sodium STATIC ${sodium_SRC})
target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include)
target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include/sodium)
target_include_directories(sodium PUBLIC ${CMAKE_CURRENT_LIST_DIR}/libsodium-config)
target_include_directories(sodium PUBLIC ${CMAKE_CURRENT_LIST_DIR}/libsodium-config/sodium)
target_compile_options(sodium PRIVATE -DCONFIGURED=1)
target_link_libraries(husarnet_core sodium)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.10.5
)
set(JSON_BuildTests OFF)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(husarnet_core nlohmann_json)

FetchContent_Declare(
  better_enums
  GIT_REPOSITORY https://github.com/aantron/better-enums.git
  GIT_TAG 0.11.3
)
FetchContent_MakeAvailable(better_enums)
target_include_directories(husarnet_core PUBLIC ${better_enums_SOURCE_DIR})
target_compile_options(husarnet_core PUBLIC -DBETTER_ENUMS_STRICT_CONVERSION=1)

FetchContent_Declare(
  sqlite3
  URL https://sqlite.org/2022/sqlite-amalgamation-3390200.zip
)
FetchContent_MakeAvailable(sqlite3)
include_directories(${sqlite3_SOURCE_DIR})
add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_link_libraries(husarnet_core sqlite3 ${CMAKE_DL_LIBS})

# Include unix port libraries
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  FetchContent_Declare(
    c-ares
    GIT_REPOSITORY https://github.com/c-ares/c-ares.git
    GIT_TAG cares-1_18_1
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

# Enable HTTP control API
if(${BUILD_HTTP_CONTROL_API})
  add_compile_definitions(HTTP_CONTROL_API)

  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.10.9
  )
  set(BUILD_SHARED_LIBS OFF)
  set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF)
  set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF)
  set(HTTPLIB_REQUIRE_OPENSSL OFF)
  set(HTTPLIB_REQUIRE_ZLIB OFF)
  set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF)
  set(HTTPLIB_REQUIRE_BROTLI OFF)
  set(HTTPLIB_COMPILE OFF)
  FetchContent_MakeAvailable(httplib)
  target_link_libraries(husarnet_core httplib)

  # include_directories(${httplib_SOURCE_DIR})
  if(IS_DIRECTORY "${httplib_SOURCE_DIR}")
    set_property(DIRECTORY ${httplib_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
  endif()
endif()

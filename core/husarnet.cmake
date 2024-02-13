set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  #TODO: -fsanitize=undefined in clang
  set(COMMONFLAGS "${COMMONFLAGS} -D_GLIBCXX_DEBUG -g")
else()
  set(COMMONFLAGS "${COMMONFLAGS} -Os -ffunction-sections -fdata-sections")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
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
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -lrt")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -lpthread")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COMMONFLAGS} -Wl,--gc-sections")
endif()

if(DEFINED FAIL_ON_WARNING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wconversion")
endif()

if (DEFINED IDF_TARGET)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

# Configure the build
set(BUILD_HTTP_CONTROL_API FALSE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Windows) OR(${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
  set(BUILD_HTTP_CONTROL_API TRUE)
endif()

# Build IDF framework libraries
# TODO: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-spiram-cache-workaround
# if (DEFINED IDF_TARGET)
#   #include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
#   set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#   set(CMAKE_COLOR_DIAGNOSTICS ON)
#   idf_build_process("${IDF_TARGET}"
#                     COMPONENTS lwip nvs_flash freertos esp_netif esp_timer cxx esp_hw_support esp_wifi
#                     SDKCONFIG ${SDKCONFIG}
#                     #SDKCONFIG_DEFAULTS ${SDKCONFIG}.default
#                     BUILD_DIR ${CMAKE_BINARY_DIR})
# endif()

# Add all required headers and source files
list(APPEND husarnet_core_SRC) # This is more of a define rather than an append)
list(APPEND husarnet_core_include_DIRS) # This is more of a define rather than an append)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_linux_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/linux/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_linux_SRC} ${port_shared_unix_windows_SRC})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/windows)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/windows/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_windows_SRC} ${port_shared_unix_windows_SRC})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/macos)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_macos_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/macos/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_macos_SRC} ${port_shared_unix_windows_SRC})
endif()

if (DEFINED IDF_TARGET)
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports/esp32)
  list(APPEND husarnet_core_include_DIRS ${COMPONENT_DIR})
  file(GLOB port_esp32_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/esp32/*.cpp")
  list(APPEND husarnet_core_SRC ${port_esp32_SRC})
endif()

list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/ports)
file(GLOB husarnet_ports_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/*.cpp")
list(APPEND husarnet_core_SRC ${husarnet_ports_SRC})

if(${BUILD_HTTP_CONTROL_API})
  list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR}/api_server)
  file(GLOB api_server_SRC "${CMAKE_CURRENT_LIST_DIR}/api_server/*.cpp")
  list(APPEND husarnet_core_SRC ${api_server_SRC})
endif()

# Top level project files
list(APPEND husarnet_core_include_DIRS ${CMAKE_CURRENT_LIST_DIR})

file(GLOB core_SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
list(APPEND husarnet_core_SRC ${core_SRC})

# "Aliasing" (copying) core folder as "husarnet"
# So includes like "husarnet/something.h" do work too
set(TEMP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/tempIncludes)
file(MAKE_DIRECTORY ${TEMP_INCLUDE_DIR})
file(CREATE_LINK ${CMAKE_CURRENT_LIST_DIR}/ ${TEMP_INCLUDE_DIR}/husarnet SYMBOLIC)

# Join all of the above
if(DEFINED IDF_TARGET)
  idf_component_register(
    SRCS ${husarnet_core_SRC}
    INCLUDE_DIRS ${husarnet_core_include_DIRS}
    REQUIRES lwip nvs_flash esp_netif esp_wifi libsodium)
elseif()
  add_library(husarnet_core STATIC ${husarnet_core_SRC})
  include_directories(${husarnet_core_include_DIRS})
endif()

# IDF (ESP32) build system doesn't allow us to change 
# component/library name and adds "idf::" prefix to it
if(DEFINED IDF_TARGET)
  set(husarnet_core ${COMPONENT_LIB})
elseif()
  set(husarnet_core "husarnet_core")
endif()

target_include_directories(${husarnet_core} PUBLIC ${TEMP_INCLUDE_DIR})
target_compile_definitions(${husarnet_core} PRIVATE PORT_ARCH="${CMAKE_SYSTEM_PROCESSOR}")

# Propagate selected ESP32 target for compile time checks
# Disable unknown pragmas warning as we are not using clang in ESP32 builds for now
if (DEFINED IDF_TARGET)
  target_compile_definitions(${husarnet_core} PRIVATE IDF_TARGET=${IDF_TARGET})
  target_compile_options(${husarnet_core} PRIVATE -Wno-unknown-pragmas -Wno-missing-field-initializers)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${husarnet_core} PRIVATE DEBUG_BUILD=1)
endif()

# Configure dependencies
include(FetchContent)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
  target_link_libraries(${husarnet_core} iphlpapi shlwapi ws2_32)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Android)
  target_link_libraries(${husarnet_core} log)
endif()

#TODO: during ESP32 build use native ESP32 libsodium component
if (NOT DEFINED IDF_TARGET)
  FetchContent_Declare(
    libsodium
    GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
    GIT_TAG 1.0.19
  )
  FetchContent_MakeAvailable(libsodium)

  # Generate compile-time version header
  # Needs to be populated manually from libsodium's configure.ac
  set(SODIUM_LIBRARY_VERSION_STRING "1.0.19")

  set(SODIUM_LIBRARY_VERSION_MAJOR 28)
  set(SODIUM_LIBRARY_VERSION_MINOR  0)
  include(${CMAKE_CURRENT_LIST_DIR}/lib-config/libsodium/generate.cmake)

  include_directories(${libsodium_SOURCE_DIR}/src/libsodium/include)
  file(GLOB_RECURSE sodium_SRC ${libsodium_SOURCE_DIR}/src/libsodium/*.c)
  add_library(sodium STATIC ${sodium_SRC})
  target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include)
  target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include/sodium)
  target_compile_options(sodium PRIVATE -DCONFIGURED=1 -Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  target_link_libraries(${husarnet_core} stdc++)
endif()

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
set(JSON_BuildTests OFF)
FetchContent_MakeAvailable(nlohmann_json)
target_include_directories(${husarnet_core} PUBLIC ${nlohmann_json_SOURCE_DIR}/include)
target_link_libraries(${husarnet_core} nlohmann_json)

FetchContent_Declare(
  better_enums
  GIT_REPOSITORY https://github.com/aantron/better-enums.git
  GIT_TAG 0.11.3
)
FetchContent_MakeAvailable(better_enums)
target_include_directories(${husarnet_core} PUBLIC ${better_enums_SOURCE_DIR})
target_compile_options(${husarnet_core} PUBLIC -DBETTER_ENUMS_STRICT_CONVERSION=1)

if((${CMAKE_SYSTEM_NAME} STREQUAL Linux) OR(${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  FetchContent_Declare(
    sqlite3
    URL https://sqlite.org/2023/sqlite-amalgamation-3440000.zip
  )
  FetchContent_MakeAvailable(sqlite3)
  include_directories(${sqlite3_SOURCE_DIR})
  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_link_libraries(${husarnet_core} sqlite3 ${CMAKE_DL_LIBS})
  target_compile_options(${husarnet_core} PUBLIC -DENABLE_LEGACY_CONFIG=1)
endif()

# Include linux port libraries
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
  FetchContent_Declare(
    c-ares
    GIT_REPOSITORY https://github.com/c-ares/c-ares.git
    GIT_TAG cares-1_22_0
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
  target_link_libraries(${husarnet_core} c-ares)
endif()

# Build libnl without its build system to prevent autoconf madness
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  FetchContent_Declare(
    libnl
    GIT_REPOSITORY https://github.com/thom311/libnl.git
    GIT_TAG libnl3_9_0

    # Patch specific to the build system (only applies to zig v0.9)
    # More details are in the patch file
    PATCH_COMMAND git apply ${CMAKE_CURRENT_LIST_DIR}/lib-config/libnl/socket.c.patch
    UPDATE_DISCONNECTED TRUE
  )

  FetchContent_MakeAvailable(libnl)

  # Locking is handled by a port-wide mutex around libnl calls. Pthread
  # rwlock unlock sometimes caused UB traps being triggered on zig v0.9
  set(LIBNL_ENABLE_PTHREADS OFF) 
  set(LIBNL_ENABLE_DEBUG OFF)

  # Generate compile-time version header
  # Needs to be populated manually from libnl's configure.ac
  set(LIBNL_VER_MAJ 3)
  set(LIBNL_VER_MIN 9)
  set(LIBNL_VER_MIC 0)

  set(LIBNL_CURRENT  226)
  set(LIBNL_REVISION 0)
  set(LIBNL_AGE      26)
  include(${CMAKE_CURRENT_LIST_DIR}/lib-config/libnl/generate.cmake)

  # Generate Bison/Flex files
  find_package(BISON)
  find_package(FLEX)

  set(BISON_FLAGS "-Wno-deprecated -Wno-other")

  bison_target(
    libnl_ematch_syntax ${libnl_SOURCE_DIR}/lib/route/cls/ematch_syntax.y
    ${libnl_SOURCE_DIR}/lib/route/cls/ematch_syntax.c
    DEFINES_FILE ${libnl_SOURCE_DIR}/lib/route/cls/ematch_syntax.h
    COMPILE_FLAGS ${BISON_FLAGS}
  )
  bison_target(
    libnl_pktloc_syntax ${libnl_SOURCE_DIR}/lib/route/pktloc_syntax.y
    ${libnl_SOURCE_DIR}/lib/route/pktloc_syntax.c
    DEFINES_FILE ${libnl_SOURCE_DIR}/lib/route/pktloc_syntax.h
    COMPILE_FLAGS ${BISON_FLAGS}
  )
  flex_target(
    libnl_ematch_grammar ${libnl_SOURCE_DIR}/lib/route/cls/ematch_grammar.l
    ${libnl_SOURCE_DIR}/lib/route/cls/ematch_grammar.c
    DEFINES_FILE ${libnl_SOURCE_DIR}/lib/route/cls/ematch_grammar.h
  )
  flex_target(
    libnl_pkloc_grammar ${libnl_SOURCE_DIR}/lib/route/pktloc_grammar.l
    ${libnl_SOURCE_DIR}/lib/route/pktloc_grammar.c
    DEFINES_FILE ${libnl_SOURCE_DIR}/lib/route/pktloc_grammar.h
  )

  add_flex_bison_dependency(libnl_ematch_grammar libnl_ematch_syntax)
  add_flex_bison_dependency(libnl_pkloc_grammar libnl_pktloc_syntax)

  # Alias version.h file to make the library happy
  file(CREATE_LINK ${libnl_SOURCE_DIR}/include/netlink/version.h ${libnl_SOURCE_DIR}/include/version.h SYMBOLIC)

  file(GLOB_RECURSE libnl_SOURCES "${libnl_SOURCE_DIR}/lib/*.c")
  
  include_directories(
    ${libnl_SOURCE_DIR}/include
    ${libnl_SOURCE_DIR}/lib/genl
    ${libnl_SOURCE_DIR}/lib/idiag
    ${libnl_SOURCE_DIR}/lib/netfilter
    ${libnl_SOURCE_DIR}/lib/route
    ${libnl_SOURCE_DIR}/lib/xfrm)

  add_library(libnl STATIC
    ${libnl_SOURCES}
    ${BISON_libnl_ematch_syntax_OUTPUTS}
    ${BISON_libnl_pktloc_syntax_OUTPUTS}
    ${FLEX_libnl_ematch_grammar_OUTPUTS}
    ${FLEX_libnl_pkloc_grammar_OUTPUTS}
  )
  #target_link_libraries(libnl STATIC ${FLEX_LIBRARIES})

  if(NOT ${LIBNL_ENABLE_PTHREADS})
    add_compile_definitions(DISABLE_PTHREADS)
    message(STATUS "Disabling pthreads in libnl")
  else()
    target_compile_options(libnl PRIVATE -lpthread)
    message(STATUS "Enabling pthreads in libnl")
  endif()

  if(${LIBNL_ENABLE_DEBUG})
    add_compile_definitions(LIBNL_NL_DEBUG)
    message(STATUS "Enabling debug in libnl")
  endif()
  
  target_compile_definitions(libnl PRIVATE SYSCONFDIR="/etc/libnl" _GNU_SOURCE)
  target_compile_options(libnl PRIVATE -Wno-unused-variable)

  target_link_libraries(husarnet_core libnl)
endif()

# Enable HTTP control API
if(${BUILD_HTTP_CONTROL_API})
  add_compile_definitions(HTTP_CONTROL_API)

  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
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
  target_link_libraries(${husarnet_core} httplib)

  # include_directories(${httplib_SOURCE_DIR})
  if(IS_DIRECTORY "${httplib_SOURCE_DIR}")
    set_property(DIRECTORY ${httplib_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
  endif()
endif()

# if (DEFINED IDF_TARGET)
#   target_link_libraries(husarnet_core
#     idf::lwip idf::nvs_flash idf::freertos idf::esp_netif idf::esp_timer idf::cxx idf::esp_hw_support idf::esp_wifi)
#   target_compile_definitions(husarnet_core PRIVATE IDF_TARGET=${IDF_TARGET})

#   # Add dummy target to make the ESP-IDF build system happy
#   add_executable(${CMAKE_PROJECT_NAME}.elf ${CMAKE_CURRENT_LIST_DIR}/ports/esp32/main.c)
#   target_link_libraries(${CMAKE_PROJECT_NAME}.elf husarnet_core)
#   idf_build_executable(${CMAKE_PROJECT_NAME}.elf)
# endif()

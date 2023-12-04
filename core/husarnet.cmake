set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(COMMONFLAGS "${COMMONFLAGS} -D_GLIBCXX_DEBUG -g -fsanitize=undefined -fsanitize=undefined") # -fsanitize=thread

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")
# set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "include-what-you-use")
else()
  set(COMMONFLAGS "${COMMONFLAGS} -O3 -ffunction-sections -fdata-sections")
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

# Configure the build
set(BUILD_HTTP_CONTROL_API FALSE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux OR(${CMAKE_SYSTEM_NAME} STREQUAL Windows) OR(${CMAKE_SYSTEM_NAME} STREQUAL Darwin))
  set(BUILD_HTTP_CONTROL_API TRUE)
endif()

# Add all required headers and source files
list(APPEND husarnet_core_SRC) # This is more of a define rather than an append

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows)
  file(GLOB port_linux_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/linux/*.cpp")
  file(GLOB port_shared_unix_windows_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/shared_unix_windows/*.cpp")
  list(APPEND husarnet_core_SRC ${port_linux_SRC} ${port_shared_unix_windows_SRC})
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

if(${CMAKE_SYSTEM_NAME} STREQUAL ESP32)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/ports/esp32)
  file(GLOB port_esp32_SRC "${CMAKE_CURRENT_LIST_DIR}/ports/esp32/*.cpp")
  list(APPEND husarnet_core_SRC ${port_esp32_SRC})
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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(husarnet_core PRIVATE DEBUG_BUILD=1)
endif()

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
  GIT_TAG 1.0.19
)
FetchContent_MakeAvailable(libsodium)
include_directories(${libsodium_SOURCE_DIR}/src/libsodium/include)
file(GLOB_RECURSE sodium_SRC ${libsodium_SOURCE_DIR}/src/libsodium/*.c)
add_library(sodium STATIC ${sodium_SRC})
target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include)
target_include_directories(sodium PUBLIC ${libsodium_SOURCE_DIR}/src/libsodium/include/sodium)
target_include_directories(sodium PUBLIC ${CMAKE_CURRENT_LIST_DIR}/libsodium-config)
target_include_directories(sodium PUBLIC ${CMAKE_CURRENT_LIST_DIR}/libsodium-config/sodium)
target_compile_options(sodium PRIVATE -DCONFIGURED=1 -Wno-unused-function -Wno-unknown-pragmas -Wno-unused-variable)

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  target_link_libraries(husarnet_core stdc++)
endif()

target_link_libraries(husarnet_core sodium)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
set(JSON_BuildTests OFF)
FetchContent_MakeAvailable(nlohmann_json)
target_include_directories(husarnet_core PUBLIC ${nlohmann_json_SOURCE_DIR}/include)
target_link_libraries(husarnet_core nlohmann_json)

FetchContent_Declare(
  better_enums
  GIT_REPOSITORY https://github.com/aantron/better-enums.git
  GIT_TAG 0.11.3
)
FetchContent_MakeAvailable(better_enums)
target_include_directories(husarnet_core PUBLIC ${better_enums_SOURCE_DIR})
target_compile_options(husarnet_core PUBLIC -DBETTER_ENUMS_STRICT_CONVERSION=1)

if((${CMAKE_SYSTEM_NAME} STREQUAL Linux) OR(${CMAKE_SYSTEM_NAME} STREQUAL Windows))
  FetchContent_Declare(
    sqlite3
    URL https://sqlite.org/2023/sqlite-amalgamation-3440000.zip
  )
  FetchContent_MakeAvailable(sqlite3)
  include_directories(${sqlite3_SOURCE_DIR})
  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_link_libraries(husarnet_core sqlite3 ${CMAKE_DL_LIBS})
  target_compile_options(husarnet_core PUBLIC -DENABLE_LEGACY_CONFIG=1)
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
  target_link_libraries(husarnet_core c-ares)
endif()

# Build libnl without its build system to prevent autoconf madness 
if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  FetchContent_Declare(
    libnl
    GIT_REPOSITORY https://github.com/thom311/libnl.git
    GIT_TAG libnl3_8_0
  )

  FetchContent_MakeAvailable(libnl)

  # Pthreads lib causes an undefined behaviour trap
  # being triggered when unlocking rwlock
  #TODO: fix or introduce global nllib mutex
  set(LIBNL_ENABLE_PTHREADS OFF) 
  set(LIBNL_ENABLE_DEBUG OFF)

  # Generate compile-time version header
  # Needs to be populated manually from libnl's configure.ac
  set(LIBNL_VER_MAJ 3)
  set(LIBNL_VER_MIN 8)
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
  target_link_libraries(husarnet_core httplib)

  # include_directories(${httplib_SOURCE_DIR})
  if(IS_DIRECTORY "${httplib_SOURCE_DIR}")
    set_property(DIRECTORY ${httplib_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
  endif()
endif()

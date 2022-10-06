cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(husarnet VERSION "0.0.0")

include(../core/husarnet.cmake)

file(GLOB husarnet_tests_SRC "${CMAKE_CURRENT_LIST_DIR}/husarnet/*.cpp")
add_executable(husarnet_tests ${husarnet_tests_SRC})

target_compile_definitions(husarnet_core PRIVATE UT=1)
target_link_libraries(husarnet_tests husarnet_core)

# "Aliasing" (copying) core folder as "husarnet"
# So includes like "husarnet/something.h" do work too
set(TEMP_INCLUDE_DIR ${CMAKE_BINARY_DIR}/tempIncludes)
file(MAKE_DIRECTORY ${TEMP_INCLUDE_DIR})
file(COPY ../core/ DESTINATION ${TEMP_INCLUDE_DIR}/husarnet/ FILES_MATCHING PATTERN "*.h")

target_include_directories(husarnet_tests PUBLIC ${TEMP_INCLUDE_DIR})

if(IS_DIRECTORY "${husarnet_core_SOURCE_DIR}")
  set_property(DIRECTORY ${husarnet_core_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL YES)
endif()

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.8
)
FetchContent_MakeAvailable(Catch2)
target_link_libraries(husarnet_tests Catch2::Catch2)

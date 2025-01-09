cmake_minimum_required(VERSION 3.10)

project(pir_scheme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
# Disable C++ extensions
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# output compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -g -O0 -Wall -march=native")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -march=native")

option(EXTRA_TESTS "EXTRA_TESTS: enable extra tests" ON)

message(STATUS "CMAKE_BUILD_TYPE:   ${CMAKE_BUILD_TYPE}")
message(STATUS "EXTRA_TESTS:        ${EXTRA_TESTS}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

# Use FetchContent for HEXL
include(FetchContent)
FetchContent_Declare(
    hexl
    GIT_REPOSITORY https://github.com/intel/hexl.git
    GIT_TAG v1.2.5
)

# Configure HEXL build options before fetching
set(HEXL_TESTING OFF CACHE BOOL "")
set(HEXL_BENCHMARK OFF CACHE BOOL "")
set(HEXL_COVERAGE OFF CACHE BOOL "")
set(HEXL_DOCS OFF CACHE BOOL "")

# Fetch and make HEXL available
FetchContent_MakeAvailable(hexl)

message(STATUS "HEXL will be built from source using FetchContent")

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

add_executable(test-first-dimension ${PROJECT_SOURCE_DIR}/tests/test-first-dimension.cpp)
target_include_directories(test-first-dimension PUBLIC ${PROJECT_SOURCE_DIR}/src)

# HEXL::hexl target will be available after FetchContent_MakeAvailable
target_link_libraries(test-first-dimension PRIVATE pir HEXL::hexl)

if (EXTRA_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()
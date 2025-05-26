cmake_minimum_required(VERSION 3.10)

project(pir_scheme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
# Disable C++ extensions
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# output compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -g -O0 -mavx2")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -mavx2")

option(EXTRA_TESTS "EXTRA_TESTS: enable extra tests" ON)

message(STATUS "CMAKE_BUILD_TYPE:   ${CMAKE_BUILD_TYPE}")
message(STATUS "EXTRA_TESTS:        ${EXTRA_TESTS}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")

include(cmake/dependencies.cmake)
include(cmake/compile_options.cmake)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

if (EXTRA_TESTS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()
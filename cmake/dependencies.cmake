
set(FETCHCONTENT_QUIET OFF)  # 显示下载进度
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)  # 断点续传
set(FETCHCONTENT_TIMEOUT 6000)  # 全局设置超时时间为600秒

set(FETCHCONTENT_DOWNLOAD_EXTRACT_TIMESTAMP true)
# cmake_policy(SET CMP0135 NEW)

set(git_clone_shallow_options "--depth 1 --single-branch")

function(CustomFetch name)
    set(options)
    set(oneValueArgs GIT_REPOSITORY GIT_TAG GIT_DEPTH)
    set(multiValueArgs)
    cmake_parse_arguments(FETCHCONTENT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    FetchContent_Declare(
        ${name}
        GIT_REPOSITORY ${FETCHCONTENT_GIT_REPOSITORY}
        GIT_TAG ${FETCHCONTENT_GIT_TAG}
        GIT_PROGRESS TRUE
        GIT_SHALLOW 1
        GIT_DEPTH 1
    )
endfunction()

include(FetchContent)

CustomFetch(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.12.0 # Using the version tag you want to fetch
)
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif()

CustomFetch(
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

CustomFetch(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# Configure GTest build options
set(INSTALL_GTEST OFF CACHE BOOL "")
set(BUILD_GMOCK OFF CACHE BOOL "")

FetchContent_MakeAvailable(googletest)

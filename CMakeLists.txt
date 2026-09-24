cmake_minimum_required(VERSION 3.14)

project(YCSBC LANGUAGES C CXX)

# Keep the benchmark executable at the top level of the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

include(FetchContent)
find_package(Threads REQUIRED)

# Hiredis is a project dependency, rather than a system library.  Fetching it
# as a static CMake target keeps the build independent of /usr/local and avoids
# running an install command with elevated privileges.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build YCSB-C dependencies as static libraries" FORCE)
set(DISABLE_TESTS OFF CACHE BOOL "Enable hiredis tests" FORCE)
set(ENABLE_SSL OFF CACHE BOOL "Disable hiredis SSL support" FORCE)
set(ENABLE_EXAMPLES OFF CACHE BOOL "Disable hiredis examples" FORCE)

FetchContent_Declare(
  hiredis
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG v1.4.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(hiredis)

if (TARGET hiredis-test)
  set_target_properties(hiredis-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
  )
endif()

# The original project expects TBB. Prefer an installed TBB package, but keep
# a source fallback so that a fresh checkout can be built without installing
# another development package first.
option(YCSBC_FETCH_TBB "Fetch oneTBB when no system TBB package is available" ON)
find_package(TBB CONFIG QUIET)
if (NOT TBB_FOUND)
  find_package(TBB QUIET)
endif()

if (NOT TARGET TBB::tbb)
  if (NOT YCSBC_FETCH_TBB)
    message(FATAL_ERROR
      "TBB was not found. Install oneTBB or configure with "
      "-DYCSBC_FETCH_TBB=ON.")
  endif()

  set(TBB_TEST OFF CACHE BOOL "Disable oneTBB tests" FORCE)
  set(TBB_EXAMPLES OFF CACHE BOOL "Disable oneTBB examples" FORCE)
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Force oneTBB to build as shared" FORCE)
  set(TBB_STRICT OFF CACHE BOOL "Disable oneTBB warnings-as-errors" FORCE)
  set(TBB_ENABLE_IPO OFF CACHE BOOL "Disable oneTBB IPO" FORCE)
  set(TBB_INSTALL OFF CACHE BOOL "Disable oneTBB install rules" FORCE)
  set(TBB_FIND_PACKAGE OFF CACHE BOOL "Build the fetched oneTBB" FORCE)
  set(TBBMALLOC_BUILD OFF CACHE BOOL "Do not build oneTBB malloc" FORCE)
  set(TBB_BUILD ON CACHE BOOL "Build oneTBB" FORCE)

  FetchContent_Declare(
    onetbb
    GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
    GIT_TAG v2022.3.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(onetbb)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build YCSB-C dependencies as static libraries" FORCE)
endif()

if (NOT TARGET TBB::tbb)
  message(FATAL_ERROR "The selected TBB package does not provide TBB::tbb.")
endif()

file(GLOB YCSBC_CORE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/core/*.cc")
file(GLOB YCSBC_DB_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/db/*.cc")

add_executable(ycsbc
  ycsbc.cc
  ${YCSBC_CORE_SOURCES}
  ${YCSBC_DB_SOURCES}
)

target_compile_features(ycsbc PRIVATE cxx_std_11)
set_target_properties(ycsbc PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}"
)
target_include_directories(ycsbc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(ycsbc PRIVATE
  Threads::Threads
  hiredis::hiredis
  TBB::tbb
)

# Workload files and helper scripts are copied next to the executable so the
# benchmark can be launched from the build directory without referring back to
# the source tree.
file(COPY
  "${CMAKE_CURRENT_SOURCE_DIR}/workloads"
  "${CMAKE_CURRENT_SOURCE_DIR}/run.sh"
  "${CMAKE_CURRENT_SOURCE_DIR}/run_redis.sh"
  DESTINATION "${CMAKE_BINARY_DIR}"
)

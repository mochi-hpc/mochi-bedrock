# (C) 2020 The University of Chicago
# See COPYRIGHT in top-level directory.
cmake_minimum_required (VERSION 3.8)
project (bedrock C CXX)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing ()


option (ENABLE_COVERAGE "Enable code coverage" OFF)
option (ENABLE_TESTS    "Build tests" OFF)
option (ENABLE_EXAMPLES "Build examples" OFF)
option (ENABLE_MPI      "Enable MPI support" OFF)
option (ENABLE_FLOCK    "Enable Flock support" OFF)
option (ENABLE_PYTHON   "Enable Python support" OFF)

# library version set here (e.g. for shared libs).
set (BEDROCK_VERSION_MAJOR 0)
set (BEDROCK_VERSION_MINOR 15)
set (BEDROCK_VERSION_PATCH 0)
set (BEDROCK_VERSION
     "${BEDROCK_VERSION_MAJOR}.${BEDROCK_VERSION_MINOR}.${BEDROCK_VERSION_PATCH}")
add_definitions ("-DBEDROCK_VERSION=${BEDROCK_VERSION}")

# add our cmake module directory to the path
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# link shared lib with full rpath
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE RelWithDebInfo
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()
set (CMAKE_PREFIX_PATH "" CACHE STRING "External dependencies path")
set (BUILD_SHARED_LIBS "ON" CACHE BOOL "Build a shared library")

add_library (coverage_config INTERFACE)
if (ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options (coverage_config INTERFACE
        -O0        # no optimization
        -g         # generate debug info
        --coverage # sets all required flags
    )
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options (coverage_config INTERFACE --coverage)
    else ()
        target_link_libraries (coverage_config INTERFACE --coverage)
    endif ()
endif ()

# search for bedrock-module-api
find_package (bedrock-module-api REQUIRED)
# search fo thallium
find_package (thallium REQUIRED)
# search for nlohmann/json and schema validator
find_package (nlohmann_json REQUIRED)
find_package (nlohmann_json_schema_validator REQUIRED)
# search for TCLAP
find_package (TCLAP REQUIRED)
# search for spdlog
find_package (spdlog REQUIRED)
# search for fmt
find_package (fmt REQUIRED)
# search for toml11
find_package (toml11 REQUIRED)
# search for abt-io
if (ENABLE_FLOCK)
    find_package (flock REQUIRED)
    add_definitions (-DENABLE_FLOCK)
    set (OPTIONAL_FLOCK flock::client)
    set (OPTIONAL_SERVER_DEPS "${OPTIONAL_SERVER_DEPS} flock-client")
    set (OPTIONAL_CLIENT_DEPS "${OPTIONAL_CLIENT_DEPS} flock-client")
endif ()
# search for MPI
if (${ENABLE_MPI})
    find_package (MPI REQUIRED)
    add_definitions (-DENABLE_MPI)
    set (OPTIONAL_MPI MPI::MPI_C)
endif ()
# search for Python
if (ENABLE_PYTHON)
    set (Python3_FIND_STRATEGY LOCATION)
    find_package (Python3 COMPONENTS Interpreter Development REQUIRED)
    find_package (pybind11 REQUIRED)
    add_subdirectory (python)
endif ()

add_subdirectory (bin)
add_subdirectory (src)

if (ENABLE_EXAMPLES)
    add_subdirectory (examples)
endif ()

if (ENABLE_TESTS)
    enable_testing ()
    find_package (Catch2 QUIET)
    if (NOT Catch2_FOUND)
      include (FetchContent)
      FetchContent_Declare (
          Catch2
          GIT_REPOSITORY https://github.com/catchorg/Catch2.git
          GIT_TAG        v3.6.0
      )
      FetchContent_MakeAvailable (Catch2)
    endif ()
    add_subdirectory (tests)
endif ()

# (C) 2020 The University of Chicago 
# See COPYRIGHT in top-level directory.
cmake_minimum_required (VERSION 3.8)
project (bedrock C CXX)
set (CMAKE_CXX_STANDARD 14)
enable_testing ()

add_definitions (-g -Wextra -Wall -Wpedantic)

option (ENABLE_TESTS    "Build tests" OFF)
option (ENABLE_EXAMPLES "Build examples" OFF)

# add our cmake module directory to the path
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# link shared lib with full rpath
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set (CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# setup cache variables for ccmake
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE Release
         CACHE STRING "Choose the type of build." FORCE)
    set_property (CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                  "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif ()
set (CMAKE_PREFIX_PATH "" CACHE STRING "External dependencies path")
set (BUILD_SHARED_LIBS "ON" CACHE BOOL "Build a shared library")

find_package (PkgConfig REQUIRED)
# search fo thallium
find_package (thallium REQUIRED)
# search for abt-io
pkg_check_modules (ABTIO REQUIRED IMPORTED_TARGET abt-io)
# search for ssg
pkg_check_modules (SSG REQUIRED IMPORTED_TARGET ssg)
# search for nlohmann/json
find_package (nlohmann_json REQUIRED)
# search for TCLAP
find_package (TCLAP REQUIRED)
# search for spdlog
find_package(spdlog REQUIRED)

# library version set here (e.g. for shared libs).
set (BEDROCK_VERSION_MAJOR 0)
set (BEDROCK_VERSION_MINOR 1)
set (BEDROCK_VERSION_PATCH 0)
set (BEDROCK_VERSION
     "${BEDROCK_VERSION_MAJOR}.${BEDROCK_VERSION_MINOR}.${BEDROCK_VERSION_PATCH}")

add_subdirectory (bin)
add_subdirectory (src)
if(${ENABLE_TESTS})
  find_package (CppUnit REQUIRED)
  add_subdirectory (tests)
endif(${ENABLE_TESTS})
if(${ENABLE_EXAMPLES})
  add_subdirectory (examples)
endif(${ENABLE_EXAMPLES})

# set source files
set (server-src-files
     Server.cpp
     MargoManager.cpp
     ProviderManager.cpp
     DependencyFinder.cpp
     MargoLogging.cpp
     Jx9Manager.cpp
     MPIEnv.cpp)

set (client-src-files
     Client.cpp
     ServiceHandle.cpp
     ServiceGroupHandle.cpp
     AsyncRequest.cpp)

set (jx9-src-files
     jx9/jx9.c)

# jx9 library
add_library (jx9 STATIC ${jx9-src-files})
set_property (TARGET jx9 PROPERTY POSITION_INDEPENDENT_CODE ON)

# load package helper for generating cmake CONFIG packages
include (CMakePackageConfigHelpers)

# where to install files for "find_package"
set (bedrock-pkg "share/cmake/bedrock")

# set version fo shared library
set (bedrock-vers "${BEDROCK_VERSION_MAJOR}.${BEDROCK_VERSION_MINOR}")

# server library
add_library (bedrock-server ${server-src-files})
add_library (bedrock::server ALIAS bedrock-server)
target_compile_options (bedrock-server PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries (bedrock-server
    PRIVATE
    nlohmann_json_schema_validator::validator
    toml11::toml11 jx9 coverage_config
    PUBLIC
    bedrock::module-api
    thallium
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    fmt::fmt
    ${OPTIONAL_MPI}
    ${OPTIONAL_FLOCK})
target_include_directories (bedrock-server PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (bedrock-server BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (bedrock-server BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (bedrock-server
    PROPERTIES VERSION ${BEDROCK_VERSION}
    SOVERSION ${BEDROCK_VERSION_MAJOR})

# client library
add_library (bedrock-client ${client-src-files})
add_library (bedrock::client ALIAS bedrock-client)
target_compile_options (bedrock-client PUBLIC -Wall -Wextra -Wpedantic)
target_link_libraries (bedrock-client
    PRIVATE coverage_config
    ${OPTIONAL_FLOCK}
    PUBLIC
    bedrock::module-api
    nlohmann_json::nlohmann_json
    thallium
    spdlog::spdlog
    fmt::fmt)
target_include_directories (bedrock-client PUBLIC $<INSTALL_INTERFACE:include>)
target_include_directories (bedrock-client BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
target_include_directories (bedrock-client BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (bedrock-client
    PROPERTIES VERSION ${BEDROCK_VERSION}
    SOVERSION ${BEDROCK_VERSION_MAJOR})

# installation stuff (packaging and install commands)
write_basic_package_version_file (
    "bedrock-config-version.cmake"
    VERSION ${BEDROCK_VERSION}
    COMPATIBILITY AnyNewerVersion)

# generate our config file for find_package()
set (INCLUDE_INSTALL_DIR include/)
set (LIB_INSTALL_DIR lib/)
configure_package_config_file (bedrock-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bedrock-config.cmake
  INSTALL_DESTINATION ${LIB_INSTALL_DIR}/bedrock/cmake
  PATH_VARS INCLUDE_INSTALL_DIR)

# some bits for the pkg-config file
set (DEST_DIR "${CMAKE_INSTALL_PREFIX}")
set (SERVER_PRIVATE_LIBS "-lbedrock-server")
set (CLIENT_PRIVATE_LIBS "-lbedrock-client")
configure_file ("bedrock-server.pc.in" "bedrock-server.pc" @ONLY)
configure_file ("bedrock-client.pc.in" "bedrock-client.pc" @ONLY)

# configure config.h
configure_file ("config.h.in" "config.h" @ONLY)

# "make install" rules
install (TARGETS bedrock-server bedrock-client
         EXPORT bedrock-targets
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
install (EXPORT bedrock-targets
         DESTINATION ${bedrock-pkg}
         FILE "bedrock-targets.cmake")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/bedrock-config.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/bedrock-config-version.cmake"
         DESTINATION ${bedrock-pkg} )
install (DIRECTORY ../include/bedrock
         DESTINATION include
         FILES_MATCHING PATTERN "*.hpp")
install (DIRECTORY ../include/bedrock
         DESTINATION include
         FILES_MATCHING PATTERN "*.h")
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/bedrock-server.pc"
               "${CMAKE_CURRENT_BINARY_DIR}/bedrock-client.pc"
         DESTINATION "lib/pkgconfig/")

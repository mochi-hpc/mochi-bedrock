file (GLOB test-sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file (GLOB test-module-sources ${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cpp)

configure_file (ValidConfigs.json ValidConfigs.json COPYONLY)
configure_file (InvalidConfigs.json InvalidConfigs.json COPYONLY)

foreach (test-module-source ${test-module-sources})
    get_filename_component (name ${test-module-source} NAME_WE)
    add_library (${name} SHARED ${test-module-source})
    target_link_libraries (${name} PRIVATE bedrock-server coverage_config)
endforeach ()

foreach (test-source ${test-sources})
    get_filename_component (name ${test-source} NAME_WE)
    add_executable (Test${name} ${test-source})
    target_link_libraries (Test${name} PRIVATE Catch2::Catch2WithMain bedrock-server bedrock-client coverage_config)
    add_test (NAME Test${name} COMMAND ./Test${name})
endforeach ()

file (GLOB test-sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

configure_file (ValidConfigs.json ValidConfigs.json COPYONLY)
configure_file (InvalidConfigs.json InvalidConfigs.json COPYONLY)

foreach (test-source ${test-sources})
    get_filename_component (name ${test-source} NAME_WE)
    add_executable (Test${name} ${test-source})
    target_link_libraries (Test${name} PRIVATE Catch2::Catch2WithMain bedrock-server bedrock-client coverage_config)
    add_test (NAME Test${name} COMMAND ./Test${name})
endforeach ()

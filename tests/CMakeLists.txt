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

# Set the path to the python directory
set (PYTHON_MODULE_DIR ${CMAKE_SOURCE_DIR}/python)
# Use file(GLOB_RECURSE ...) to find all files matching the test_*.py pattern
file (GLOB_RECURSE PYTHON_TEST_FILES "${PYTHON_MODULE_DIR}/test_*.py")

foreach (PYTHON_TEST_FILE ${PYTHON_TEST_FILES})
    # Remove the directory part
    file (RELATIVE_PATH PYTHON_TEST_FILE_REL ${PYTHON_MODULE_DIR} ${PYTHON_TEST_FILE})
    # Remove the file extension
    string (REPLACE ".py" "" PYTHON_TEST_FILE_BASE ${PYTHON_TEST_FILE_REL})
    # Replace slashes with dots
    string (REPLACE "/" "." PYTHON_TEST_NAME ${PYTHON_TEST_FILE_BASE})
    # Add the test
    if (${ENABLE_COVERAGE})
        message (STATUS "${PYTHON_TEST_NAME} test will run with code coverage")
        add_test (NAME ${PYTHON_TEST_NAME} COMMAND coverage run -m unittest ${PYTHON_TEST_NAME})
    else ()
        add_test (NAME ${PYTHON_TEST_NAME} COMMAND python -m unittest ${PYTHON_TEST_NAME})
    endif ()
    set_property (TEST ${PYTHON_TEST_NAME} PROPERTY ENVIRONMENT
                  PYTHONPATH=${CMAKE_SOURCE_DIR}/python/:${CMAKE_BINARY_DIR}/python:$ENV{PYTHONPATH})
endforeach()


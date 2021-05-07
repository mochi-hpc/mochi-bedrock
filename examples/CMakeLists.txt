add_executable (example-client ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
target_link_libraries (example-client bedrock-client)

add_library (example-module-a ${CMAKE_CURRENT_SOURCE_DIR}/module-a.c)
target_include_directories (example-module-a PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries (example-module-a bedrock-server)

add_library (example-module-b ${CMAKE_CURRENT_SOURCE_DIR}/module-b.cpp)
target_include_directories (example-module-b PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries (example-module-b bedrock-server)

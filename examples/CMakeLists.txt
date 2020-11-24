#add_executable (example-server ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp)
#target_link_libraries (example-server bedrock-server)

#add_executable (example-admin ${CMAKE_CURRENT_SOURCE_DIR}/admin.cpp)
#target_link_libraries (example-admin bedrock-admin)

#add_executable (example-client ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp)
#target_link_libraries (example-client bedrock-client)

add_library (example-module-a ${CMAKE_CURRENT_SOURCE_DIR}/module-a.c)
target_include_directories (example-module-a PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_library (example-module-b ${CMAKE_CURRENT_SOURCE_DIR}/module-b.cpp)
target_include_directories (example-module-b PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

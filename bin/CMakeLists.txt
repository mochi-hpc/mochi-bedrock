add_executable (bedrock ${CMAKE_CURRENT_SOURCE_DIR}/bedrock.cpp)
target_link_libraries (bedrock bedrock-server)

install (TARGETS bedrock DESTINATION bin)

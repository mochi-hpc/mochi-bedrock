add_executable (bedrock ${CMAKE_CURRENT_SOURCE_DIR}/bedrock.cpp)
target_link_libraries (bedrock bedrock-server)

add_executable (bedrock-query ${CMAKE_CURRENT_SOURCE_DIR}/bedrock-query.cpp)
target_link_libraries (bedrock-query bedrock-client ${OPTIONAL_SSG})

add_executable (bedrock-shutdown ${CMAKE_CURRENT_SOURCE_DIR}/bedrock-shutdown.cpp)
target_link_libraries (bedrock-shutdown bedrock-client ${OPTIONAL_SSG})

install (TARGETS bedrock DESTINATION bin)
install (TARGETS bedrock-query DESTINATION bin)
install (TARGETS bedrock-shutdown DESTINATION bin)

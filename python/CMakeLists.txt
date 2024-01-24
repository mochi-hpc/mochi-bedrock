add_library (pybedrock_client MODULE src/py-bedrock-client.cpp)
target_link_libraries (pybedrock_client PRIVATE pybind11::module bedrock-client PRIVATE coverage_config)
pybind11_extension (pybedrock_client)
pybind11_strip (pybedrock_client)

add_library (pybedrock_server MODULE src/py-bedrock-server.cpp)
target_link_libraries (pybedrock_server PRIVATE pybind11::module bedrock-server PRIVATE coverage_config)
pybind11_extension (pybedrock_server)
pybind11_strip (pybedrock_server)

set (PY_VERSION ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR})

install (TARGETS pybedrock_client pybedrock_server
         EXPORT bedrock-targets
         ARCHIVE DESTINATION lib/python${PY_VERSION}/site-packages
         LIBRARY DESTINATION lib/python${PY_VERSION}/site-packages)

install (DIRECTORY mochi/bedrock
         DESTINATION lib/python${PY_VERSION}/site-packages/mochi)

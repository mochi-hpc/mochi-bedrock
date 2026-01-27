# Build all examples
set (EXAMPLES
     basic_client
     load_module
     add_pool
     add_xstream
     add_provider
     change_pool
     migrate
     snapshot
     restore
     async
     query
     jx9_query
     complete
)

foreach (example ${EXAMPLES})
    add_executable (07_bedrock_${example} ${example}.cpp)
    target_link_libraries (07_bedrock_${example} bedrock::client)
endforeach ()

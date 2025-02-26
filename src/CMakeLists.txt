add_executable(protoc-gen-dpb
    main.cpp
)
target_include_directories(protoc-gen-dpb
    PRIVATE
        .
)
target_link_libraries(protoc-gen-dpb
    protobuf::libprotobuf
    protobuf::libprotoc
)

# install
install(TARGETS protoc-gen-dpb)

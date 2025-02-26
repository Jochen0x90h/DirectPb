cmake_minimum_required(VERSION 3.19)

# set project
project(directpb)

# configure languages
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)


# dependencies
find_package(protobuf CONFIG)


add_subdirectory(src)

# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(src)

# Include the header files into the source files.
include_directories(${CMAKE_SOURCE_DIR}/include)

# Recursively add the subdirectories' CMake files as well.
add_subdirectory(graphs)
add_subdirectory(visitors)
add_subdirectory(passes)
add_subdirectory(programs)
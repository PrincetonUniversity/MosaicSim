# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the root directory.
project(apollo)

# Include the header files into the entire project.
include_directories("headers")

# Recursively add the subdirectories' CMake files as well.
add_subdirectory(graphs)
add_subdirectory(visitors)
add_subdirectory(passes)
add_subdirectory(programs)

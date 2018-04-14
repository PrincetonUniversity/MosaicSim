# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the root directory.
project(apollo)

# Find all of the required configurations needed for LLVM in this project.
find_package(LLVM REQUIRED CONFIG)

# Add the LLVM definitions and include directories to build the project.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Include the header files into the entire project.
include_directories("headers")

# Recursively add the subdirectories' CMake files as well.
add_subdirectory(graphs)
add_subdirectory(visitors)
add_subdirectory(passes)
add_subdirectory(programs)

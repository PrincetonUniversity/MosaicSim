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

# Recursively add the source files into the entire project.
add_subdirectory(src)

# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Set the compiler to be clang-based as opposed to gcc-based.
if (APPLE)
  set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
  set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
#elseif (LINUX)
#  set(CMAKE_C_COMPILER "/usr/local/bin/clang")
#  set(CMAKE_CXX_COMPILER "/usr/local/bin/clang++")
endif (APPLE)

# Add functionality to handle LLVM bitcode generation in CMake more simply.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/utils")

# Name of the root directory.
project(apollo)

# Find all of the required configurations needed for LLVM in this project.
find_package(LLVM REQUIRED CONFIG)

# Add the LLVM definitions and include directories to build the project.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Also add the link-time libraries to resolve all the symbol dependencies.
link_directories(lib)

# Recursively add the source files into the entire project.
add_subdirectory(src)

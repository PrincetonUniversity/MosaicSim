cmake_minimum_required(VERSION 3.10)

# Set the compiler to emit libraries and binaries to the appropriate directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Name of the root directory.
project(pythia)

# Find all of the required configurations needed for LLVM in this project.
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Add the LLVM definitions and include directories to build the project.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Recursively add the source files into the entire project.
add_subdirectory(${CMAKE_SOURCE_DIR}/pass)
add_subdirectory(${CMAKE_SOURCE_DIR}/sim)

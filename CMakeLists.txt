# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the root directory.
project(apollo)

# Recursively add the pass and programs subdirectories' CMake files as well.
add_subdirectory(passes)
add_subdirectory(programs)

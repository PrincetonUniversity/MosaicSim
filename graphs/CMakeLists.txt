# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(graphs)

# Add the files to be compiled from this subdirectory.
file(GLOB GRAPH_SOURCES "*.cpp")

# Create a run-time library based on these source files from above.
add_library(graphs STATIC ${GRAPH_SOURCES})

# Use C++14 to compile the pass.
# Build the LLVM without any C++ run-time type information (RTTI).
set_target_properties(
  graphs
  PROPERTIES
    COMPILE_FLAGS "-std=c++14 -fno-rtti"
)
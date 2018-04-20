# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(passes)

# Add the files (passes) to be compiled from this subdirectory.
file(GLOB PASS_SOURCES "*.cpp")

# Create a run-time library out of the various LLVM passes from above.
add_library(DependenceGraph MODULE ${PASS_SOURCES})

# Use C++14 to compile the pass.
# Build the LLVM without C++ run-time type information (RTTI). IMPORTANT!!
# Also build the .so library file without any prefixes (e.g. 'lib').
set_target_properties(
  DependenceGraph
  PROPERTIES
    COMPILE_FLAGS "-std=c++14 -fno-rtti"
    PREFIX        ""
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)

# Get proper shared-library behavior (where symbols are not necessarily resolved
# when the shared library is linked) on Mac OS X.
if (APPLE)
  set_target_properties(
    DependenceGraph
    PROPERTIES
      LINK_FLAGS  "-undefined dynamic_lookup"
  )
endif (APPLE)
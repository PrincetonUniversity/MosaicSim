# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(passes)

# Find all of the required configurations needed for LLVM in this project.
find_package(LLVM REQUIRED CONFIG)

# Add the files (passes) to be compiled from this subdirectory.
file(GLOB PASS_SOURCES "*.cpp")

# Create a run-time library out of the various LLVM passes from above.
add_library(passes MODULE ${PASS_SOURCES})

# Add the LLVM definitions and include directories to build the project.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Use C++14 to compile the pass.
# Build the LLVM with C++ run-time type information (RTTI).
# Also build the .so library file without any prefixes (e.g. 'lib').
set_target_properties(
  passes
  PROPERTIES
    COMPILE_FLAGS "-std=c++14"
    PREFIX        ""
)

# Specify where the shared-library pass files should be created.
install(TARGETS passes DESTINATION lib)

# Get proper shared-library behavior (where symbols are not necessarily resolved
# when the shared library is linked) on Mac OS X.
if (APPLE)
  set_target_properties(
    passes
    PROPERTIES
      LINK_FLAGS  "-undefined dynamic_lookup"
  )
endif(APPLE)
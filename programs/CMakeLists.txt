# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(programs)

# Add the files to be compiled from this subdirectory.
file(GLOB PROGRAM_SOURCES "*.cpp")

# Create a run-time library based on these source files from above.
add_library(programs STATIC ${PROGRAM_SOURCES})

# Use C++14 to compile the pass.
# Build the LLVM without any C++ run-time type information (RTTI).
set_target_properties(
  programs
  PROPERTIES
    COMPILE_FLAGS "-std=c++14 -fno-rtti"
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)

# Get proper shared-library behavior (where symbols are not necessarily resolved
# when the shared library is linked) on Mac OS X.
if (APPLE)
  set_target_properties(
    programs
    PROPERTIES
      LINK_FLAGS  "-undefined dynamic_lookup"
  )
endif (APPLE)
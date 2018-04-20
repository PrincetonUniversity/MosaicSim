# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(visitors)

# Add the files to be compiled from this subdirectory.
file(GLOB VISITOR_SOURCES "*.cpp")

# Create a run-time library based on these source files from above.
add_library(visitors STATIC ${VISITOR_SOURCES})

# Use C++14 to compile the pass.
# Also build the .a library file without any prefixes (e.g. 'lib').
set_target_properties(
  visitors
  PROPERTIES
    COMPILE_FLAGS "-std=c++14"
    PREFIX        ""
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)

# Get proper shared-library behavior (where symbols are not necessarily resolved
# when the shared library is linked) on Mac OS X.
if (APPLE)
  set_target_properties(
    visitors
    PROPERTIES
      LINK_FLAGS  "-undefined dynamic_lookup"
  )
endif (APPLE)
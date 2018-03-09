# Adds the performance file to the LLVM pass.
add_library(PerformancePass MODULE
  Performance.cpp
)

# Use C++17 to compile the pass (i.e., supply -std=c++17).
set_property(TARGET PerformancePass PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET PerformancePass PROPERTY CXX_STANDARD 17)

# Build the LLVM without any C++ run-time type information (RTTI).
set_target_properties(PerformancePass PROPERTIES
  COMPILE_FLAGS "-fno-rtti"
)

# Build the .so library file without any prefixes (e.g. 'lib').
set_target_properties(PerformancePass PROPERTIES PREFIX "")

# Get proper shared-library behavior (where symbols are not necessarily resolved
# when the shared library is linked) on Mac OS X.
if(APPLE)
  set_target_properties(PerformancePass PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
  )
endif(APPLE)
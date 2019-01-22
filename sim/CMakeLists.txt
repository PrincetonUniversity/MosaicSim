LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_CXX_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

if(FALSE)
  set(CMAKE_CXX_FLAGS_DEBUG "-DNDEBUG")
endif()

if(FALSE)
  set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
ADD_EXECUTABLE(sim main.cc sim.cc core/Core.cc memsys/Cache.cc memsys/DRAM.cc core/LoadStoreQ.cc graph/Graph.cc core/DynamicNode.cc misc/Config.cc power_model/aladdin_power_model.cpp)
target_link_libraries(sim dramsim cacti pthread)

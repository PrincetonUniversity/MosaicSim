LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_CXX_STANDARD 11)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

if(FALSE)
  set(CMAKE_CXX_FLAGS_DEBUG "-DNDEBUG")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
ADD_EXECUTABLE(sim main.cc sim.cc tile/Core.cc memsys/Cache.cc memsys/SimpleDRAM.cc memsys/DRAM.cc tile/LoadStoreQ.cc graph/Graph.cc tile/DynamicNode.cc tile/Accelerator.cc tile/accelerator_models/c-model/gemm/gemm_model.cpp tile/accelerator_models/c-model/sdp/sdp_model.cpp tile/accelerator_models/c-model/nvdla/nvdla.cpp  misc/Config.cc power_model/aladdin_power_model.cpp)

target_link_libraries(sim dramsim cacti pthread)

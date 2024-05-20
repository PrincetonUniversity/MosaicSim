# Define sources for the MosaicSim library
add_library(MosaicSim
  sim.cpp
  sim.hpp
  parms.cpp
  parms.hpp
  memsys/CommMec.hpp
  memsys/Cache.hpp
  memsys/Cache.cpp
  memsys/L1Cache.cpp
  memsys/L2Cache.cpp
  memsys/LLCache.cpp
  memsys/SimpleDRAM.cpp
  memsys/SimpleDRAM.hpp
  memsys/DRAM.cpp
  memsys/DRAM.hpp
  memsys/FunctionalCache.cpp
  memsys/FunctionalCache.hpp
  graph/Graph.cpp
  graph/Graph.hpp
  tile/Tile.hpp
  tile/Core.cpp
  tile/Core.hpp
  tile/LoadStoreQ.cpp
  tile/LoadStoreQ.hpp
  tile/DynamicNode.cpp
  tile/DynamicNode.hpp
  tile/DESCQ.cpp
  tile/DESCQ.hpp
  tile/Bpred.cpp
  tile/Bpred.hpp
  tile/Accelerator.cpp
  tile/Accelerator.hpp
  tile/accelerator_models/c-model/gemm/gemm_model.cpp
  tile/accelerator_models/c-model/sdp/sdp_model.cpp
  tile/accelerator_models/c-model/nvdla/nvdla.cpp
  misc/Config.cpp
  misc/Config.hpp
  misc/Memory_helpers.cpp
  misc/Memory_helpers.hpp
  misc/Config.hpp
  misc/Statistics.hpp
  misc/Statistics.cpp)

# Add the DRAMSim2 directory for linking 
link_directories(${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2)

# Create the executable 
ADD_EXECUTABLE(mosaicSim_sim sim_driver.cpp) 

# Create the executable 
ADD_EXECUTABLE(mosaicSim_compile compiler_driver.cpp) 

# Create the executable 
ADD_EXECUTABLE(mosaicSim mosaic_driver.cpp) 

# Compile DRAMSIM2 library 
add_custom_command(
  TARGET mosaicSim_sim PRE_BUILD
  COMMAND make libdramsim.so
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2/)
# Compile DRAMSIM2 library 
add_custom_command(
  TARGET mosaicSim_compile PRE_BUILD
  COMMAND make libdramsim.so
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2/)
# Compile DRAMSIM2 library 
add_custom_command(
  TARGET mosaicSim PRE_BUILD
  COMMAND make libdramsim.so
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2/)

# Link with the additional libraries
target_link_libraries(mosaicSim_sim MosaicSim dramsim pthread stdc++fs OpenMP::OpenMP_CXX)
target_link_libraries(mosaicSim_compile MosaicSim dramsim pthread stdc++fs OpenMP::OpenMP_CXX)
target_link_libraries(mosaicSim  MosaicSim dramsim pthread stdc++fs OpenMP::OpenMP_CXX)

# Add the tests
add_subdirectory(tests)

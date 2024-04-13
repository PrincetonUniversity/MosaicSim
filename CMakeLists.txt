cmake_minimum_required(VERSION 3.10)

# Name of the Project
project(MosaicSim CXX)

# Activate testing 
ENABLE_TESTING()

#set c++  standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS " -march=native ")
set(CMAKE_CXX_FLAGS_DEBUG " -ggdb -O0 ")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 ")

#Ugly checker for the filesystem (not all compiler/version have it) 
include(CheckIncludeFileCXX)
set(HAVE_FS FALSE)
check_include_file_cxx("filesystem" HAVE_STD_FILESYSTEM)
check_include_file_cxx("experimental/filesystem" HAVE_STD_EXPERIMENTAL_FILESYSTEM)
if(HAVE_STD_FILESYSTEM)
  set(HAVE_FS TRUE) 
  set(EXPERIMENTAL_FILESYSTEM 0)
elseif(HAVE_STD_EXPERIMENTAL_FILESYSTEM)
  set(HAVE_FS TRUE) 
  set(EXPERIMENTAL_FILESYSTEM 1)
endif()
if(NOT HAVE_FS)
  message(FATAL_ERROR "Could not find C++ filesystem. Please use a more recent compiler")
endif()

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Fetch DRAMSim2 if needed 
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2/Makefile)
  execute_process(COMMAND git submodule update --init --recursive ${CMAKE_SOURCE_DIR}/sim/memsys/DRAMSim2/)
endif()

# Find LLVM.
set(LLVM_REQ_VERSION "11.0" CACHE STRING "Requested LLVM Version.") 
find_package(LLVM ${LLVM_REQ_VERSION} REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

#Find the OpenMP library
find_package(OpenMP)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp") 
#find DEC++
if(LOCAL_DEC)
  set(DEC ${PROJECT_BINARY_DIR}/compiler/DEC/src/DEC++-build/bin/DEC++)
else()  
  find_program(DEC DEC++)
  if (DEC STREQUAL "DEC-NOTFOUND")
    message(FATAL_ERROR "DEC++ not found. Please update your PATH environment variable to include DEC++.")
  endif()
  message(STATUS "DEC++: ${DEC}")
endif()

option(DUMP_ASCI_INPUT "Dumps dynamic input into ASCI files." OFF)
if(DUMP_ASCI_INPUT)
  add_definitions(-DDUMP_ASCI_INPUT=1)
endif()

# find doxygen
find_package(Doxygen)

# needed for the config.h
include_directories(${PROJECT_BINARY_DIR}/include)
set(PREPROC_PIPES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/preproc_pipes.sh)
set(PREPROC_FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/preproc_files.sh)
set(MOSAIC_CONFIG_DIR ${PROJECT_BINARY_DIR}/config)

# Adds the compilation rules into the project.
add_subdirectory(sim)
add_subdirectory(compiler)
add_subdirectory(config)

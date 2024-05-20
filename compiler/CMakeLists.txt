# Add the LLVM link and include directories to build the project.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Adding the libGraphGen 
add_library(GraphGen MODULE GraphGen.cpp)
set_target_properties(GraphGen PROPERTIES COMPILE_FLAGS "-fno-rtti")
if(APPLE)
    set_target_properties(GraphGen PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

# Adding the libRecordDynamicInfo
add_library(RecordDynamicInfo MODULE RecordDynamicInfo.cpp)
set_target_properties(RecordDynamicInfo PROPERTIES COMPILE_FLAGS "-fno-rtti")
if(APPLE)
  set_target_properties(RecordDynamicInfo PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

find_package(OpenMP REQUIRED)

set(TRACER tracer_pipes.llvm)
configure_file(preproc.sh ${PREPROC_PIPES} @ONLY)
set(TRACER tracer_files.llvm)
configure_file(preproc.sh ${PREPROC_FILES} @ONLY)
  
# compile tracer.cc  together with all 
# TODO: find a way to compile the tracer.cc in camke correctly 
# sources: https://github.com/compor/llvm-ir-cmake-utils
# TODO2: is clang++ in there when only llvm package is installed?

if(DUMP_ASCI_INPUT)
add_custom_target(tracer_pipes.llvm ALL
  ${LLVM_TOOLS_BINARY_DIR}/clang++ ${OpenMP_CXX_FLAGS} -S -O3 -emit-llvm -DDUMP_ASCI_INPUT=1 -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_pipes.llvm
  ${CMAKE_CURRENT_SOURCE_DIR}/tracer.cc ${OpenMP_EXE_LINKER_FLAGS} 
  BYPRODUCTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_pipes.llvm)

add_custom_target(tracer_files.llvm ALL
  ${LLVM_TOOLS_BINARY_DIR}/clang++ ${OpenMP_CXX_FLAGS} -S -O3 -emit-llvm -DDUMP_ASCI_INPUT=1 -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_files.llvm
  ${CMAKE_CURRENT_SOURCE_DIR}/tracer.cc ${OpenMP_EXE_LINKER_FLAGS} -DUSE_FILES=1
  BYPRODUCTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_files.llvm)
else()
add_custom_target(tracer_pipes.llvm ALL
  ${LLVM_TOOLS_BINARY_DIR}/clang++ ${OpenMP_CXX_FLAGS} -S -O3 -emit-llvm -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_pipes.llvm
  ${CMAKE_CURRENT_SOURCE_DIR}/tracer.cc ${OpenMP_EXE_LINKER_FLAGS} 
  BYPRODUCTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_pipes.llvm)

add_custom_target(tracer_files.llvm ALL
  ${LLVM_TOOLS_BINARY_DIR}/clang++ ${OpenMP_CXX_FLAGS} -S -O3 -emit-llvm -o ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_files.llvm
  ${CMAKE_CURRENT_SOURCE_DIR}/tracer.cc ${OpenMP_EXE_LINKER_FLAGS} -DUSE_FILES=1
  BYPRODUCTS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/tracer_files.llvm)
endif()

if(LOCAL_DEC)
  INCLUDE(ExternalProject)
  ExternalProject_Add(DEC++
    PREFIX DEC
    DOWNLOAD_COMMAND git clone git@github.com:PrincetonUniversity/DECADES_compiler_private.git
    CMAKE_ARGS ../DECADES_compiler_private -DLLVM_REQ_VERSION=${LLVM_REQ_VERSION}
    INSTALL_COMMAND ""  )
endif()

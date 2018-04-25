# Not a very backwards-compatible requirement, but neither are LLVM releases.
cmake_minimum_required(VERSION 3.10)

# Name of the subdirectory.
project(programs)

# Add the files to be compiled from this subdirectory.
file(GLOB_RECURSE PROGRAM_SOURCES "*.cpp")

# Loop over all of the sources in the directory.
foreach(program ${PROGRAM_SOURCES})
  # Create a base file name out of the source file.
  get_filename_component(base ${program} NAME_WE)

  # Set the appropriate arguments for the custom compilation command.
  set(COMPILER clang++)
  set(FLAGS -std=c++11 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -S)
  set(SRC ${program})
  set(OUTPUT ${CMAKE_SOURCE_DIR}/bin/${base}.ll)

  # Builds the appropriate LLVM IR file out of the source/base file.
  add_custom_command(
    OUTPUT  ${OUTPUT}
    COMMAND ${COMPILER} ${FLAGS} ${SRC} -o ${OUTPUT}
    DEPENDS ${SRC}
  )

  # Create the custom target based on the output information.
  add_custom_target(
    ${base}.ll
    ALL DEPENDS
      ${OUTPUT}
  )
endforeach(program)
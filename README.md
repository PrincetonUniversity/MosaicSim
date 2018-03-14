# deca
**Decoupled architectures for customized hardware accelerators.**

NOTE: This is a work in progress and will be heavily updated and cleaned up.

# Prerequisites

This project requires the following tools:

 + A `clang`/LLVM toolchain, 5.0.0+
 + `cmake` 3.10+
 + `ninja` 1.8.2 (optional)

# Usage

To compile the LLVM pass, navigate to the root directory of the project. To compile the files, run

    cmake -G Ninja

Alternatively, run it without the flags to target `Makefile` instead. Then, a simple run of

    ninja
  
(or `make`) as needed will generate the pass in `blockperf/PerformancePass.so`. One can then run the pass on various files using the following syntax (assuming access from the root of the project):

    opt -load blockperf/PerformancePass.so -estimate < PATH_TO_SRC_FILE
  
Obviously, this is not a very good pass right now, but we hope to make it better. To generate the bitcode (`.bc`) file, one can use `clang` or `clang++`:

    clang++ -Wno-c++11-extensions -O3 -emit-llvm SRC_FILE.cpp -c -o SRC_FILE.bc

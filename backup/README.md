# apollo

**Uncovering Sources of Speedup for Performance Improvement in Specialized Hardware Accelerators**

### Prerequisites

This project was built with the following tools:

 + GNU Make 3.81+ 
 + `cmake` 3.10+
 + `ninja` 1.8.2
 + `clang`/LLVM toolchain, 6.0.0+ (with C++14 support)

No compatibility is guaranteed for other compilers/versions of these toolchains, mainly because LLVM is fairly backwards-incompatible. :-)

### Usage

To compile the LLVM pass, navigate to the root directory of the project. To compile the files, simply run

    make

Then, a simple run of

    ninja
  
in the root directory will generate the pass in the `lib` directory. One can then run the pass on various files using the following syntax (assuming access from the root of the project):

    opt -load lib/PASS.so -FLAG < PATH_TO_SRC_FILE
  
To generate a bitcode (`.bc`) file, one can use `clang` or `clang++`:

    clang++ -std=c++11 -emit-llvm -c SRC_FILE.cpp

### Sources

We used the following sources to aid in our design of this tool:

1. [https://github.com/smanilov/icsa-dswp](https://github.com/smanilov/icsa-dswp)

2. [https://github.com/ysshao/ALADDIN](https://github.com/ysshao/ALADDIN)

3. [https://github.com/compor/llvm-ir-cmake-utils](https://github.com/compor/llvm-ir-cmake-utils)

4. [https://github.com/S2E/tools/blob/master/LLVMBitcode.cmake](https://github.com/S2E/tools/blob/master/LLVMBitcode.cmake)

5. J. Ferrante, K. J. Ottenstein, and J. D. Warren, "The program dependence graph and its use in optimization," in *Symposium on Programming*, 1984.
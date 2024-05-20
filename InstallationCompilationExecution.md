# MosaicSim

MosaicSim is an LLVM-based lightweight modular simulator for heterogeneous systems. It offers accuracy and agility and is designed specifically for hardware-software co-design explorations. 

## Dependecies

This project depends on the following packages:

* CMake 3.10 or higher
* CLANG V11.1.0
* LLVM V11.1.0
* [DECADES Compiler](https://github.com/PrincetonUniversity/DecadesCompiler)

All the dependencies, except for the DECADES Compiler, can be installed using a package manager. For example, on a Ubuntu platform, the following line:

```console
sudo apt install cmake clang-11
````

will install all the needed packages.

The DECADES Compiler can be  installed either manually or thought the compilation process of the MosaicSim. If you have already installed the DECADES compiler from the MosaicSim source directory, execute the following commands:

```console
mkdir build
cd build
cmake  ..
make
```

If you wish to fetch and compile the DECADES compiler at the same time as MosaicSim, pass the "-DLOCAL_DEC=1" parameter  to the CMake command:

```console
    cmake -DLOCAL_DEC=1 ..
```

In this case, the DECADES compiler creates shared libraries. For MosaicSim to be able to use these libraries, the LD_LIBRARY_PATH environment variable should be updated to contain the libraries directory:

```console
    export LD_LIBRARY_PATH+=:/path/to/MosaicSim/build/compiler/DEC/src/DEC++-build/lib/
```

Once everything is set up, to ensure that the software is running properly, invoke "ctest" from the build directory. This will automatically launch a set of tests to ensure correct behavior.


## Using MosaicSim executables

This package provides three executables: sim_driver, compiler_driver and mosaic. compiler_driver to compiles the C++ source file, the sim_driver to simulate your executable, and mosaic does both. For example:  

```console
mosaic --num_cores 2 --decoupled test.cpp     
```

will compile and simulate the test.cpp file in decoupling mode with four cores, two supply/consume pairs. The equivalent of the previous command using the sim_driver and compiler_driver would be:

```console
compiler_driver --num_cores 2 --decoupled  test.cpp
sim_driver --num_cores 2 --decoupled 
```

When mosaic is used, pipes are used for the inter-process communication between the native run and the simulator, while when compiler_driver and sim_driver are used, files are used instead. 

With its modular approach, MosaicSim offers different configurations for the simulator, the cores, and the DRAM memory. These options are exposed through the executables with the --sim-config, --core-config --RAM-config (note that --DRAM-config takes two arguments: system and device) arguments. They can either take a predefined option provided by the simulator, or a path to a file written in the same format.
Run each executable with the "--help" or "-h" argument to explore all available options. 


## Documentation

This package relies on the Doxygen package for the documentation. To generate the documentation, execute:

```console
make docs
```

from the build directory. If CMake does not detect the Doxygen package, this target will be disabled.
## Statistics

The simulator outputs simulation statistics directly to the console (e.g., # cycles, # cache misses, # dram accesses, IPC, etc). MosaicSim can be run with an optional -o flag to specify a directory for these stats. By default, these files are wrtitten in the current directory.



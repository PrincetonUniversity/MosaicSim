# MosaicSim

MosaicSim is an LLVM-based lightweight modular simulator for heterogeneous systems. It offers accuracy and agility and is designed specifically for hardware-software co-design explorations. 

## Dependecies

This project depends on the following packages:

* CMake 3.10 or higher
* CLANG V11.1.0
* LLVM V11.1.0
* [DECADES Compiler](https://github.com/PrincetonUniversity/DecadesCompiler)

All the dependecies, except for the DECADES Compiler, can be installed using a package manager. For example, on a Ubuntu platform, the following line:

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

If you wish to fetch and compile the DECADES compiler at the same time as this package, pass the "-DLOCAL_DEC=1" parameter  to the CMake command:

```console
    cmake -DLOCAL_DEC=1 ..
```

Additionally, the DECADES compiler creates shared libraries. For MosaicSim to be able to use these libraries, the LD_LIBRARY_PATH environment variable should be updated to contain the libraries directory:

```console
    export LD_LIBRARY_PATH+=:/path/to/MosaicSim/build/compiler/DEC/src/DEC++-build/lib/
```

Once everything is set up, in order to make sure that the software is running properly, invoke "ctest" from the build directory. This will automatically launch a set of tests ensuring correct behavior.


## Using MosaicSim executables

This package provides three executables: sim_driver, compiler_driver and mosaic. compiler_driver to compiles the C++ source file, the sim_driver to simulate your executable, and mosaic does both. For example:  

```console
mosaic --num_cores 2 --decoupled test.cpp     
```

will compile and simulate the test.cpp file using four cores: two supply/consume pairs since decoupled mode is being used. The equivalent of to the previous command using the sim_driver and compiler_driver would be:

```console
compiler_driver --num_cores 2 --decoupled  test.cpp
sim_driver --num_cores 2 --decoupled 
```

With its modular approach, MosaicSim offers different configuration for the simulator, the cores that are used and the DRAM memory. These option are exposed through the executables with the --sim-config, --core-config --RAM-config (note that --DRAM-config takes two arguments: system and device) arguments. They can either take a predefined option provided by the simulator, or a file written in the same format.
Run each executable with the "--help" or "-h" argument to explore all its available options. 


## Documentation

This package relies on the Doxygen package for the documentation. To generate the documentation, execute:

```console
make docs
```

from the build directory. If CMake does not detect the Doxygen package, this target will be disabled.
## Statistics

The simulator outputs simulation statistics directly to the console (e.g., # cycles, # cache misses, # dram accesses, IPC, etc). MosaicSim can be run with an optional -o flag to specify a directory for these stats. MosaicSim also prints out additional statistics to specific files, outputted to the provided directory name or the run directory, if none is provided. 


## License

  [BSD License (BSD 2-Clause License)](BSD-License.txt)

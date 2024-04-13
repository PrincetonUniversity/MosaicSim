# MosaicSim

MosaicSim is LLVM based lightweight modular simulator for heterogeneous systems, offering accuracy and agility designed specifically for hardware-software co-design explorations. 

## Quickstart

This project depends on the following packages:

* CMake 3.10 or higher
* CLANG V11.1.0
* LLVM V11.1.0
* [DECADES Compiler](https://github.com/PrincetonUniversity/DECADES_compiler_private)

All the dependecies, except for the DECADES Compiler, can be installed using a package manager. For example, on a Ubuntu platform, the following line:

```console
sudo apt install cmake clang-11
````

will install all the needed packages.

The DECADES Compiler can be  installed either manually or thought the compilation process of the MosaicSim. If you have already installed the DECADES compiler, from the MosaicSim source directory execute the following lines:

```console
mkdir build
cd build
cmake  ..
make
```

In order to fetch and compile the DECADES compiler in the same time as this package, just pass the "-DLOCAL_DEC=1" parameter  to the CMake command:

```console
    cmake -DLOCAL_DEC=1 ..
```

Additionally, the DECADES compiler creates shared libraries. In order for MosaicSim to be able to use these libraries, the LD_LIBRARY_PATH environment variable should be updated to contain the libraries directory:

```console
    export LD_LIBRARY_PATH+=:/path/to/MosaicSim/build/compiler/DEC/src/DEC++-build/lib/
```

Once everything is setup, in order to make sure that  the software is running properly, invoke "ctest" from the build directory.

Alternatively, we provide a docker image for this package. It can be obtained by running:

```console
docker pull sn3332/decades:V0
```

## Using MosaicSim executables

This package provides three executables: sim_driver, compiler_driver and mosaic. compiler_driver to compiles the C++ source file, the sim_driver to simulate your executable, and mosaic does both. For example:  

```console
mosaic --num_cores 2 --decoupled test.cpp     
```

will compile and simulate the test.cpp file using four cores, two supply/consume pairs since we are using decoupled mode. The equivalent of to the previous command using the sim_driver and compiler_driver would be:

```console
compiler_driver --num_cores 2 --decoupled  test.cpp
sim_driver --num_cores 2 --decoupled .
```

With its modular approach, MosaicSim offers different configuration for the simulator, the cores that are used and the DRAM memory. These option are exposed through the executables with the --sim-config, --core-config --RAM-config (note that --DRAM-config takes two arguments: system and device) arguments. They can either take a predefined option offered by the simulator, or a file written in the same format.
In order to explore all the available options for each executable, run it with the "--help" or "-h" argument. 

To remove restrictions due to running with limited resources, export the following variable:
    
```console
export MOSAIC_EXPERT=true
```

## Documentation

This package relies on the Doxygen package for the documentation. In order to generate the documentation, execute:

```console
make docs
```

from the build directory. If the Doxygen package was not detected by the CMake, this make target will be disabled. Additionally, in order to depict the caller/callee graph output in the documentation, Doxygen uses the graphviz package. If this is not found, then the caller/callee graphs will not be generated.

## Statistics

The simulator outputs simulation statistics directly to the console (e.g., # cycles, # cache misses, # dram accesses, IPC, etc). MosaicSim can be run with an optional -o flag to specify a directory for these stats. MosaicSim also prints out additional statistics to specific files, outputted to the provided directory name or the run directory, if none is provided. 

Statistics on runahead distances (# cycles between issues of a PRODUCE or LOAD_PRODUCE instructions and issues of corresponding CONSUME instructions) will be outputted to "decouplingStats" 

Statistics of latencies of each load instruction will be outputted to "loadStats".


## License

  [BSD License (BSD 2-Clause License)](BSD-License.txt)

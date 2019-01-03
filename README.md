# Pythia

**Uncovering Sources of Speedup for Performance Improvement in Specialized Hardware Accelerators**

### Prerequisites

This project was built with the following tools:

 + GNU Make 4.1
 + `cmake` 3.11.4
 + `ninja` 1.8.2
 + `clang`/LLVM toolchain, 7.0.0+ (with C++14 support)

No compatibility is guaranteed for other compilers/versions of these toolchains, mainly because LLVM is fairly backwards-incompatible. :-)

### Usage

To compile the LLVM pass, navigate to the root directory of the project. To compile the files, simply run

    make

to generate compilation files in the `build` directory. Then, a simple run of

    ninja -C build

in the root directory will compile the files. <location TBD>

## Running Pythia

To run Pythia, first navigate to pythia/bin.

The run syntax is:

./sim -n [num_cores] [sim_config_name] [path_to_workload_1] [core_1_config] [path_to_workload_2] [core_2_config] ... [path_to_workload_n] [core_n_config] [-v]

Here is an example:

./sim -n 2 default ../workloads/shoc/triad/ out_of_order ../workloads/shoc/triad in_order

Note:
The config files are in pythia/sim/config. The command line arguments ommit their extensions.

The "-v" for verbose mode is optional. 
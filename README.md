# Pythia

Simulator for DECADES Project

### Prerequisites

This project was built with the following tools:

 + GNU Make 4.1
 + `cmake` 3.11.4
 + `ninja` 1.8.2
 + `clang`/LLVM toolchain, 7.0.0+ (with C++14 support)

No compatibility is guaranteed for other compilers/versions of these toolchains, mainly because LLVM is fairly backwards-incompatible. :-)

### Building the Simulator

To compile the LLVM pass and other libraries, navigate to the root directory of the project. To compile the files, simply run

    make

to generate compilation files in the `build` directory. Then, a simple run of

    ninja -C build

in the root directory will compile the files. <location TBD>

## Building Workloads
Workloads must be specially prepared through some LLVM passes (to generate a data dependency graph for the simulator) and run on the host (to generate a trace of memory accesses and control flow paths). The make files for the workloads do this automatically. 

Navigate to the workload in the pythia/workloads directory. For benchmark suites, navigate to the relevant subdirectory. For example, to build triad in shoc, you'd navigate to pythia/workloads/shoc/triad. Then type:
  
    mkdir -p int output
    make
    make run 
    
"make run" may not be necessary, depending on how the makefile is written (i.e., it may already run the executable at the end)
After making and running, there should be files generated in the "int" directory and the "output" directory (which is the most important). Check that output/ctrl.txt, output/mem.txt and output/graphOutput.txt are not empty. 

## Running Pythia

To run Pythia, first navigate to pythia/bin.

The run syntax is:

    ./sim -n [num_cores] [sim_config_name] [path_to_workload_1] [core_1_config] [path_to_workload_2] [core_2_config] ... [path_to_workload_n] [core_n_config] [-v]

Here is an example:

    ./sim -n 2 default ../workloads/shoc/triad/ default ../workloads/shoc/triad default

Note:

The config files are in pythia/sim/config for different preset modes (in order, out of order, etc.). You can modify the current ones to change the size of hardware resources or create new ones. Note that the command line arguments ommit the extensions of the config files. 

The "-v" for verbose mode is optional. 

## API Documentation

In order to integrate other core or accelerator models with Pythia and have them interract together, we have documented an API. Here is the link:
https://www.overleaf.com/project/5c87bee2b8ed496eb059acfb

## Decoupling 
Below are instructions for running decoupling on Pythia. This requires setting up the DECADES compiler DEC++ (https://github.com/PrincetonUniversity/DECADES_compiler) 

cd pythia
open pass/preproc.sh and set PYTHIA_HOME to the full path to the pythia installation directory
cd workloads/shoc/triad
To compile Triad.cpp with the DECADES compiler, run:
DEC++ -spp ../../../pass/preproc.sh Triad.cpp
To execute natively and get the trace files (control flow and memory) required by Pythia:
cd decoupled
python run.py
python run_addr.py
To execute the non-decoupled version on the simulator:
cd inlined
./inlined
To run on the simulator:
cd pythia/bin

to run decoupled (no term load optimization):
./sim -n 2 default ../workloads/shoc/triad/decoupled/supply default ../workloads/shoc/triad/decoupled/compute default

to run decoupled (with term load optimization):
./sim -n 2 default ../workloads/shoc/triad/decoupled/supply_addr default ../workloads/shoc/triad/decoupled/compute default

To run non-decoupled version:
./sim -n 1 default ../workloads/shoc/triad/inlined default

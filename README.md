# Pythia

Simulator for DECADES Project

### Prerequisites

This project was built with the following tools:

 + GNU Make 4.1
 + `cmake` 3.11.4
 + `ninja` 1.8.2
 + `clang`/LLVM toolchain, 7.0.0+ (with C++14 support)
 
DECADES Compiler (DEC++): https://github.com/PrincetonUniversity/DECADES_compiler

No compatibility is guaranteed for other compilers/versions of these toolchains, mainly because LLVM is fairly backwards-incompatible.

### Building the Simulator

To compile the LLVM pass and other libraries, navigate to the root directory of the project. To compile the files, simply run

    make

to generate compilation files in the `build` directory. Then, a simple run of

    ninja -C build

in the root directory will compile the files. 

It may be convenient to add your installation directory as an environment variable. To do that, add the following line to your ~/.bashrc:
    
    export PYTHIA_HOME=/path/to/installation/directory

We'll use $PYTHIA_HOME to indicate the path below. 

## Compiling Workloads
Workloads must be specially prepared through some LLVM passes (to generate a data dependency graph for the simulator) and run on the host (to generate a trace of memory accesses and control flow paths). For this, we must use the DECADES compiler (linked above) paired with a simulator preprocessor pass/preproc.sh. Then, the binary must be run natively. 

For example, to compile the Triad benchmark in the Shoc suite, navigate to workloads/shoc/triad. Then type:
  
    DEC++ -spp $PYTHIA_HOME/pass/preproc.sh -m db Triad.cpp
    cd decades_base
    ./decades_base
      
After compiling and running, there should be files generated in a directory prefixed by "output". Check that output*/ctrl.txt, output*/mem.txt and output*/graphOutput.txt are not empty. 

## Running Pythia

To run Pythia, first navigate to $PYTHIA_HOME/bin.

The run syntax is:

    ./sim -n [num_cores] [sim_config_name] [path_to_workload_1_output] [core_1_config] [path_to_workload_2_output] [core_2_config] ... [path_to_workload_n_output] [core_n_config] [-v]

Here is an example trivially running the same code on 2 Pythia cores simultaneously:

    ./sim -n 2 sim_medium ../workloads/shoc/triad/output_compute_0 default ../workloads/shoc/triad/output_compute_0 default

Note: The "-v" for verbose mode is optional. 

CONFIGURATION FILES:

There are config files in $PYTHIA_HOME/sim/config for different preset modes (in order, out of order, perfect, etc.). You can modify the current ones to change the size of hardware resources or create new ones. Note that the command line arguments ommit the extensions of the config files. 

The first config file "sim_config_name" above gets applied to the shared L2 cache and DRAM, while the other config files "core_i_config" above are applied to the respective core i's private L1 cache and microarchitectural features. All other entries that are not applicable are simply ignored by the simulator. 

## Do-all Parallelism
To run the simulator in parallel mode (do-all parallelism), first compile for that using DEC++:

    DEC++ -spp $PYTHIA_HOME/pass/preproc.sh -m db -t 2 Triad.cpp

Execute natively (to generate Pythia's dynamic trace files):

    cd decades_base 
    ./decades_base    

Run on Pythia:

    cd $PYTHIA_HOME/bin 
    ./sim -n 2 sim_medium ../workloads/shoc/triad/decades_base/output_compute_0 default ../workloads/shoc/triad/decades_base/output_compute_1 default

## Decoupling 
To run decoupled execution in the style of "DeSC" (Ham et al., MICRO '15): 

    cd $PYTHIA_HOME/workloads/shoc/triad
Compile Triad.cpp with the DECADES compiler with the decoupled implicit flag:

    DEC++ -spp $PYTHIA_HOME/pass/preproc.sh -m di Triad.cpp

Execute natively (to generate Pythia's dynamic trace files):

    cd decades_decoupled_implicit 
    ./decades_decoupled_implicit
      
To run on the simulator:

    cd $PYTHIA_HOME/bin
    ./sim -n 2 sim_medium ../workloads/shoc/triad/decades_decoupled_implicit/output_compute_0 default ../workloads/shoc/triad/decades_decoupled_implicit/output_supply_1 default

## Statistics

After completion, the simulator outputs run statistics directly to the console (e.g., # cache misses, # dram accesses, IPC, etc). It also prints out additional statistics to specific files, described below. 

Statistics on runahead distances (# cycles between issues of a PRODUCE or LOAD_PRODUCE instructions and issues of corresponding CONSUME instructions) will be outputted to $PYTHIA_HOME/bin/decouplingStats 

Statistics of latencies of each load instruction will be outputed to a $PYTHIA_HOME/bin/loadStats.

## API Documentation

In order to integrate other core or accelerator models with Pythia and have them interract together, we have documented an API. See Section E in the linked document: https://www.overleaf.com/project/5c87bee2b8ed496eb059acfb

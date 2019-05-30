# Pythia

Simulator for DECADES Project

### Prerequisites

This project was built with the following tools:

 + GNU Make 4.1
 + `cmake` 3.11.4
 + `ninja` 1.8.2
 + `clang`/LLVM toolchain, 9.0.0+ (with C++14 support)
 + `libomp` 4.5 (e.g., to install it in CentOS type: sudo yum install llvm-toolset-7-libomp)
 
DECADES Compiler (DEC++): https://github.com/PrincetonUniversity/DECADES_compiler

No compatibility is guaranteed for older compilers/versions of these toolchains, mainly because LLVM is fairly backwards-incompatible.

### Video Demo
Here is a link to a short video that demonstrates the required steps to compile and run applications on Pythia: https://youtu.be/hUThWUhkEWg. Below, we describe the steps in more depth.

### Building the Simulator

To compile the LLVM pass and other libraries, navigate to the root directory of the project. To compile the files, simply run

    make

to generate compilation files in the `build` directory. Then, a simple run of

    ninja -C build

in the root directory will compile the files. 

Add the following line to your ~/.bashrc
    
    export PATH=[DECADES_COMPILER_INSTALL_DIR]/build/bin/:[PYTHIA_INSTALL_DIR]/tools/:$PATH
    
Your $LD_LIBRARY_PATH must be updated to always find libomp.so. Add this line to your ~/.bashrc:

    export LD_LIBRARY_PATH=[PATH_TO_OMP_SHARED_LIB]:$LD_LIBRARY_PATH

(For CentOS, [PATH_TO_OMP_SHARED_LIB] would typically be /opt/rh/llvm-toolset-7/root/usr/lib64/)

Source your bashrc to enable the change:
    
    source ~/.bashrc

## Compiling Workloads
Workloads must be specially compiled through some LLVM passes (to generate a data dependency graph for the simulator) and run on the host (to generate a trace of memory accesses and control flow paths). For this, we must use a Pythia wrapper (PDEC++) around the DECADES compiler. Then, the generated binary must be run natively.

Type PDEC++ -h for all compilation options. 

For example, to compile the graph projections benchmark for 2 threads with decoupled supply/compute, navigate to workloads/graph_projections/gp_count/decades. Then type:
       
    PDEC++ -m di -t 2 main.cpp
    cd decades_base
    ./decades_base ../../../inputs/moreno_crime/x_to_y_graph.txt
      
After compiling and running, there should be files generated in a directory prefixed by "output". Check that decades_base/output*/ctrl.txt, decades_base/output*/mem.txt and decades_base/output*/graphOutput.txt are not empty. 

## Running Pythia

Type pythiarun -h for all run options. 

To run the workload on Pythia, navigate back to the parent folder of decades_base (i.e. cd workloads/graph_projections/gp_count/decades). We must enter commandline arguments corresponding to what the workload was compiled for (for the example above, it would be decoupling with 2 threads). Type:
    
    pythiarun -n 2 -d .    

This defaults to the explicit command (core_inorder and sim_default are the default configs):

    pythiarun -n 2 -d -cc core_inorder -sc sim_default .

CUSTOM CONFIGURATION FILES:

There are config files in sim/config for the different preset modes "pythiarun -h" displays. You can modify the current ones to change the size of hardware resources or create new ones. Note that config files must be named in the form [configname].txt. 

For a number of pre-automated compilation and test runs, navigate back to workloads/graph_projections/gp_count/decades. Type:

    make

## Statistics

After completion, the simulator outputs run statistics directly to the console (e.g., # cycles, # cache misses, # dram accesses, IPC, etc). Pythia can be run with an optional -o flag to specify a directory for these stats. Pythia also prints out additional statistics to specific files, outputted to the provided directory name or the run directory, if none is provided. 

Statistics on runahead distances (# cycles between issues of a PRODUCE or LOAD_PRODUCE instructions and issues of corresponding CONSUME instructions) will be outputted to "decouplingStats" 

Statistics of latencies of each load instruction will be outputed to "loadStats".

## API Documentation

In order to integrate other core or accelerator models with Pythia and have them interract together, we have documented an API. See Section E in the linked document: https://www.overleaf.com/project/5c87bee2b8ed496eb059acfb

#!/bin/bash
#makefile to compile and run graph projections on MosaicSim simulator
#script conduct 2 runs each (1 thread, 2 threads) for non-decoupled and decoupled supply/compute
#using default config file (in order core)
#output files for run are: sim_output/base_nthreads and sim_output/decoupled_nthreads, where n is 1 or 2

CC=PDEC++
NAME=main.cpp
input_data=../../inputs/moreno_crime/x_to_y_graph.txt


mkdir -p sim_output
echo Compiling 1 Thread	
${CC} -m db -t 1 ${NAME}
echo Done compiling 1 Thread
./decades_base/decades_base ${input_data}
echo Simulating 1 Thread
mosaicrun -n 1 . 

echo Compiling Decoupled 1 Thread
${CC} -m di -t 1 ${NAME}
./decades_decoupled_implicit/decades_decoupled_implicit ${input_data}

echo Simulating Decoupled 1 Thread	
mosaicrun -d -n 1 . 
echo Compiling 2 Threads
${CC} -m db -t 2 ${NAME}
./decades_base/decades_base ${input_data}

echo Simulating 2 Threads
mosaicrun -n 2 . 

echo Compiling Decoupled 2 Threads
${CC} -m di -t 2 ${NAME}
./decades_decoupled_implicit/decades_decoupled_implicit ${input_data}

echo Simulating Decoupled 2 Threads	
mosaicrun -d -n 2 . 


#	rm -fr decades_base decades_decoupled_implicit sim_output dramsim* loadStats decouplingStats

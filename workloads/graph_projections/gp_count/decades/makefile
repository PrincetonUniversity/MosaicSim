#makefile to compile and run graph projections on Pythia simulator
#script conduct 2 runs each (1 thread, 2 threads) for non-decoupled and decoupled supply/compute
#using default config file (in order core)
#output files for run are: sim_output/base_nthreads and sim_output/decoupled_nthreads, where n is 1 or 2

CC=PDEC++
NAME=main.cpp
input_data=../../../inputs/moreno_crime/x_to_y_graph.txt

all:
	mkdir -p sim_output
	echo Compiling 1 Thread	
	$(CC) -m db -t 1 main.cpp
	cd decades_base; ./decades_base $(input_data); cd ..

	echo Simulating 1 Thread
	pythiarun -n 1 . -o sim_output/base_1thread

	echo Compiling Decoupled 1 Thread
	$(CC) -m di -t 1 main.cpp
	cd decades_decoupled_implicit; ./decades_decoupled_implicit $(input_data); cd ..

	echo Simulating Decoupled 1 Thread	
	pythiarun -d -n 1 . -o sim_output/decoupled_1thread

	echo Compiling 2 Threads
	$(CC) -m db -t 2 main.cpp
	cd decades_base; ./decades_base $(input_data); cd ..

	echo Simulating 2 Threads
	pythiarun -n 2 . -o sim_output/base_2thread

	echo Compiling Decoupled 2 Threads
	$(CC) -m di -t 2 main.cpp
	cd decades_decoupled_implicit; ./decades_decoupled_implicit $(input_data); cd ..

	echo Simulating Decoupled 2 Threads	
	pythiarun -d -n 2 . -o sim_output/decoupled_2thread

clean:
	rm -fr decades_base decades_decoupled_implicit sim_output dramsim* loadStats decouplingStats

all: base decoupled
compile: base_compile decoupled_compile
simulate: base_sim decoupled_sim

base_compile:
	compiler_driver main.cpp --exe_args "../Kron16/ 4 30175" -n 2 >  base_compile  2>&1
decoupled_compile:
	compiler_driver main.cpp --exe_args "../Kron16/ 4 30175" --decoupled -n 2 > decoupled_compile  2>&1

base_sim:
	sim_driver -n 2  input > base_sim   2>&1  &&  diff core_0 outputs/base/core_0  &&  diff core_1 outputs/base/core_1   && diff Global_sim outputs/base/Global_sim
decoupled_sim:
	sim_driver --decoupled -n 2 input > decoupled_sim  2>&1 &&  diff core_0 outputs/decoupled/core_0  &&  diff core_1 outputs/decoupled/core_1 &&  diff core_2 outputs/decoupled/core_2  &&  diff core_3 outputs/decoupled/core_3  && diff Global_sim outputs/decoupled/Global_sim

base:
	mosaic main.cpp --exe_args "../Kron16/ 4 30175" -n 2 > base 2>&1
decoupled:
	mosaic main.cpp --exe_args "../Kron16/ 4 30175" --decoupled -n 2 > decoupled  2>&1

clean:
	$(RM) -r *error.log VE_out.txt decades_* epochStats  decouplingStats memStats mlpStats queueStats base* decoupled* decoupled DeSC_dir core*  Global_sim 
	clean

all: base #desc
compile: base_compile #desc_compile
simulate: base_sim #desc_sim

base_sim: 
	sim_driver -n 2 input > base_sim  2>&1 && diff core_0 outputs/base/core_0 && diff core_1 outputs/base/core_1 && diff Global_sim outputs/base/Global_sim
desc_sim:
	sim_driver -n 2 --DeSC input > desc_sim  2>&1 &&  diff core_0 outputs/decoupled/core_0  &&  diff core_1 outputs/decoupled/core_1 &&  diff core_2 outputs/decoupled/core_2  &&  diff core_3 outputs/decoupled/core_3  && diff Global_sim outputs/decoupled/Global_sim

base_compile: 
	compiler_driver main.cpp --exe_args "../Kron16/ 4 30175" -n 2 > base_compile 2>&1 
desc_compile:
	compiler_driver main.cpp --exe_args "../Kron16/ 4 30175" --DeSC -n 2 > desc_compile 2>&1

base: 
	mosaic main.cpp --exe_args "../Kron16/ 4 30175" -n 2 > base 2>&1 
desc:
	mosaic main.cpp --exe_args "../Kron16/ 4 30175" --DeSC -n 2 > desc 2>&1

clean:
	$(RM) -r *error.log *.txt decades_* epochStats  decouplingStats memStats mlpStats queueStats base* desc* decoupled core* Global_sim
	clean

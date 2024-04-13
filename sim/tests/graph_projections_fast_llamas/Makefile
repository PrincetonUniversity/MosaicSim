all:  base desc
compile: base_compile desc_compile
simulate: base_sim desc_sim

base_compile: 
	compiler_driver main.cpp --exe_args "data.txt" -n 2 > base_compile 2>&1
desc_compile:
	compiler_driver main.cpp --exe_args "data.txt" --DeSC -n 2 > desc_compile  2>&1

base_sim: 
	sim_driver -n 2 input > base_compile 2>&1 && diff core_0 outputs/base/core_0  &&  diff core_1 outputs/base/core_1   && diff Global_sim outputs/base/Global_sim
desc_sim:
	sim_driver --DeSC -n 2 input > desc_compile 2>&1 &&  diff core_0 outputs/decoupled/core_0  &&  diff core_1 outputs/decoupled/core_1 &&  diff core_2 outputs/decoupled/core_2  &&  diff core_3 outputs/decoupled/core_3  && diff Global_sim outputs/decoupled/Global_sim 

base: 
	mosaic main.cpp --exe_args "data.txt" -n 2 > base 2>&1 
desc:
	mosaic main.cpp --exe_args "data.txt" --DeSC -n 2 > desc 2>&1

clean:
	$(RM) -r *error.log VE_out.txt decades_* epochStats  decouplingStats memStats mlpStats queueStats base* desc* decoupled DeSC_dir core* Global_sim
	clean

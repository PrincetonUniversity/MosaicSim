# Wrap around CMake and create the Ninja build files.
all:
	cmake -G Ninja -Bbuild -H.
	mkdir -p lib
	cd sim/power_model/cacti-p && make
	cd sim/memsys/DRAMSim2 && make && make libdramsim.so && cp libdramsim.so ../../../lib
	chmod +x pass/preproc.sh
	#echo "export PYTHIA_HOME="${PWD} > build/pythia_env.sh
	#chmod +x build/pythia_env.sh		
# Clean up the CMake and Ninja build files.
clean:
	find . -name "*.a"            -type f -delete
	find . -name "*.lib"          -type f -delete
	#find . -name "*.so" ! -name "libdramsim.so" -type f -delete
	find . -name "*.so"	      -type f -delete
	find . -name "*.dll"          -type f -delete
	find . -name "*.dylib"        -type f -delete
	find . -name "*.ninja*"       -type f -delete
	find . -name "*.cmake"        -type f -delete
	find . -name "CMakeCache.txt" -type f -delete
	cd sim/memsys/DRAMSim2 && make clean
	cd sim/power_model/cacti-p && make clean


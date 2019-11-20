# Wrap around CMake and create the Ninja build files.
all:
	cmake -G Ninja -Bbuild -H.
	mkdir -p lib
	cd sim/power_model/cacti-p && make
	cd sim/memsys/DRAMSim2 && make && make libdramsim.so && cp libdramsim.so ../../../lib
	cd tools && make
	chmod +x pass/preproc.sh
	chmod +x tools/mosaicrun
	chmod +x tools/PDEC++
# Clean up the CMake and Ninja build files.
clean:
	find . -name "*.a"            -type f -delete
	find . -name "*.lib"          -type f -delete
	#find . -name "*.so" ! -name "libdramsim.so" -type f -delete
	find . -name "*.so"	      -type f -delete
	find . -name "*.ll"	      -type f -delete
	find . -name "*.llvm"	      -type f -delete
	find . -name "*.dll"          -type f -delete
	find . -name "*.dylib"        -type f -delete
	find . -name "*.ninja*"       -type f -delete
	find . -name "*.cmake"        -type f -delete
	find . -name "CMakeCache.txt" -type f -delete
	cd sim/memsys/DRAMSim2 && make clean
	cd sim/power_model/cacti-p && make clean


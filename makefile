# Wrap around CMake and create the Ninja build files.
all:
	cmake -G Ninja -Bbuild -H.
	cd sim/power_model/cacti-p && make 
# Clean up the CMake and Ninja build files.
clean:
	find . -name "*.a"            -type f -delete
	find . -name "*.lib"          -type f -delete
	find . -name "*.so" ! -name "libdramsim.so" -type f -delete
	find . -name "*.dll"          -type f -delete
	find . -name "*.dylib"        -type f -delete
	find . -name "*.ninja*"       -type f -delete
	find . -name "*.cmake"        -type f -delete
	find . -name "CMakeCache.txt" -type f -delete
	cd sim/cacti-p && make clean
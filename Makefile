# Wrap around CMake and create the Ninja build files.
all:
	cmake -G Ninja

# Clean up the CMake and Ninja build files.
clean:
	find . -name "*.a"            -type f -delete
	find . -name "*.dylib"        -type f -delete
	find . -name "*.so"           -type f -delete
	find . -name "*.ninja*"       -type f -delete
	find . -name "*.cmake"        -type f -delete
	find . -name "CMakeCache.txt" -type f -delete
	find . -name "*manifest*"     -type f -delete
	find . -name "CMakeFiles"     -type d -depth -mindepth 1 -exec rm -rf "{}" \; -prune

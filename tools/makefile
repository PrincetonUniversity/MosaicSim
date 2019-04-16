CXX=clang++
OMP_INCLUDE=-I/opt/rh/devtoolset-7/root/usr/lib/gcc/x86_64-redhat-linux/7/include/

tracer: tracer.cc
	$(CXX)  -S -O3 -emit-llvm -fopenmp=libomp $(OMP_INCLUDE)  -o tracer.llvm tracer.cc
clean:
	rm tracer.llvm


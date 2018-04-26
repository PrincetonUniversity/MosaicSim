CC=clang++
TARGET_PASS=../build/graphgenerator/libgraphGen.*
PASS_NAME=graphgen
NAME=testbench
testbench: testbench.cc
	$(CC) -S -O2 -std=c++11 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $(NAME).llvm $(NAME).cc
	opt -O2 -S -disable-loop-vectorization -disable-loop-unrolling $(NAME).llvm -o $(NAME)-opt.llvm
	opt -S -instcombine -instnamer -basicaa -scev-aa -da -load $(TARGET_PASS) -$(PASS_NAME) $(NAME)-opt.llvm -o $(NAME)-pass.llvm
	opt -S -print-memdeps -analyze $(NAME)-opt.llvm

	dot -Tpng graph.dot -o output.png
	llc -filetype=obj $(NAME)-pass.llvm -o $(NAME).o
	$(CC) $(NAME).o -o $(NAME).out

clean: 
	rm -f $(NAME)
	rm *.dot
	rm *.png
	rm *.llvm
	rm *.o
	rm *.out
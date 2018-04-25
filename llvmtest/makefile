CC=clang++
TARGET_PASS=../build/graphgenerator/libgraphGen.*
PASS_NAME=graphgen
NAME=testbench
testbench: testbench.cc
	$(CC) -S -O3 -std=c++11 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $(NAME).llvm $(NAME).cc
	opt -S -instnamer -basicaa -scev-aa -print-memoryssa -load $(TARGET_PASS) -$(PASS_NAME) $(NAME).llvm -o $(NAME)-pass.llvm
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
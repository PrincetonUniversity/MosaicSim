CC=clang++
TARGET_PASS=/home/tae/dae/build/pAnalysis/libpAnalysis.*
PASS_NAME=pPass
NAME=triad
triad: triad.cc
	$(CC) -S -O3 -std=c++11 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $(NAME).llvm $(NAME).cc
	opt -S -instnamer -basicaa -scev-aa  -print-memoryssa -load $(TARGET_PASS) -$(PASS_NAME) $(NAME).llvm -o $(NAME)-pass.llvm
	llc -filetype=obj $(NAME)-pass.llvm -o $(NAME).o
	$(CC) $(NAME).o -o $(NAME).out
clean: 
	rm -f $(NAME)
	rm *.llvm
	rm *.o
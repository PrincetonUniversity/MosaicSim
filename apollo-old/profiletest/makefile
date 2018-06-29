CC=clang++
RECPASS=../build/recDynInfo/librecordDynamicInfo.*
NAME=triad
triad: triad.cc
	$(CC) -S -O3 -emit-llvm -o print.llvm trace.cc
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $(NAME).llvm $(NAME).cc
	llvm-link -S print.llvm $(NAME).llvm -o $(NAME)-integrated.llvm
	opt -S -instnamer -load $(RECPASS) -recdyninfo $(NAME)-integrated.llvm -o $(NAME)-pass.llvm
	llc -filetype=obj $(NAME)-pass.llvm -o $(NAME).o
	$(CC) $(NAME).o -o $(NAME).out
clean: 
	rm -f $(NAME)
	rm *.llvm
	rm *.o
#Polly
#opt -O3 -S -disable-slp-vectorization -disable-loop-vectorization -o $(NAME)-out.llvm $(NAME)-pass.llvm
#	opt -S -polly-canonicalize triad.llvm -o triad-polly.llvm
#	opt -S -polly-ast -analyze -q triad-polly.llvm -polly-process-unprofitable
#	opt -polly-dependences -analyze triad-polly.llvm -polly-process-unprofitable
#Default
#	$(CC) -S -O3 -fno-inline -emit-llvm -o triad-vec.llvm triad.cc
#	opt -S -instnamer -load $(RECPASS) -recdyninfo triad-vec.llvm -o triad-pass-vec.llvm
#	opt -O3 -S -instnamer -o triad-out-vec.llvm triad-pass-vec.llvm

CC=clang++
NAME=bitserial
testbench: bitserial.cpp
	$(CC) -S -O1 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME).llvm $(NAME).cpp
	llvm-link -S ~/pythia/tools/tracer.llvm int/$(NAME).llvm -o int/$(NAME)-integrated.llvm
	opt -S -instnamer -load ~/pythia/lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME)-integrated.llvm -o int/$(NAME)-pass.llvm
	opt -S -instnamer -load ~/pythia/lib/libGraphGen.so -graphgen int/$(NAME)-integrated.llvm > /dev/null		
	llc -filetype=obj int/$(NAME)-pass.llvm -o int/$(NAME).o
	llc -filetype=asm int/$(NAME)-pass.llvm -o int/$(NAME).s
	$(CC) int/$(NAME).o -o $(NAME)-profile.out
	./$(NAME)-profile.out
clean: 
	rm -f $(NAME)
	rm output/*.txt
	rm int/*.dot
	rm output/*.png
	rm *.out
	rm int/*.llvm
	rm int/*.o

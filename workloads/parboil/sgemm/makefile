CC=clang++
NAME=sgemm
testbench: sgemm.cc
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME).llvm $(NAME).cc
	llvm-link -S ../../../tools/tracer.llvm int/$(NAME).llvm -o int/$(NAME)-integrated.llvm
	opt -S -instnamer -load ../../../lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME)-integrated.llvm -o int/$(NAME)-pass.llvm
	opt -S -instnamer -load ../../../lib/libGraphGen.so -graphgen int/$(NAME)-integrated.llvm > /dev/null
	dot -Tpng int/graphDiagram.dot -o output/graphDiagram.png
	llc -filetype=obj int/$(NAME)-pass.llvm -o int/$(NAME).o
	llc -filetype=asm int/$(NAME)-pass.llvm -o int/$(NAME).s
	$(CC) int/$(NAME).o main.cc io.cc parboil.c -o $(NAME)-profile.out
	./$(NAME)-profile.out -i input/matrix1.txt,input/matrix2.txt,input/matrix2t.txt
clean: 
	rm -f $(NAME)
	rm output/*.txt
	rm int/*.dot
	rm output/*.png
	rm *.out
	rm int/*.llvm
	rm int/*.o
CC=clang++
NAME=testbench
testbench: testbench.cc
	$(CC) -static -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME).llvm $(NAME).cc
	opt -S -load ../../always_inline/build/libAlwaysInlinerPass.so -alwaysinline int/$(NAME).llvm -o int/$(NAME)-inlinepass.llvm
	llvm-link -S ../../tools/tracer.llvm  int/$(NAME)-inlinepass.llvm -o int/$(NAME)-integrated.llvm
	opt -S -instnamer -load ../../lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME)-integrated.llvm -o int/$(NAME)-pass.llvm
	opt -S -instnamer -loops -load ../../lib/libGraphGen.so -graphgen int/$(NAME)-integrated.llvm > /dev/null
	dot -Tpng int/graphDiagram.dot -o output/graphDiagram.png
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

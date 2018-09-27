CC=clang++
NAME1=compute
NAME2=supply
compute:
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME1).llvm $(NAME1).c
	llvm-link -S ../../../tools/tracer.llvm int/$(NAME1).llvm -o int/$(NAME1)-integrated.llvm
	opt -S -instnamer -load ../../../lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME1)-integrated.llvm -o int/$(NAME1)-pass.llvm
	opt -S -instnamer -loops -load ../../../lib/libGraphGen.so -graphgen int/$(NAME1)-integrated.llvm > /dev/null
	dot -Tpng int/graphDiagram.dot -o output/graphDiagram.png
	llc -filetype=obj int/$(NAME1)-pass.llvm -o int/$(NAME1).o
	llc -filetype=asm int/$(NAME1)-pass.llvm -o int/$(NAME1).s
	$(CC) int/$(NAME1).o -o $(NAME1)-profile.out
	$(CC) -g -o $(NAME1)-test.out $(NAME1).c

supply:
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME2).llvm $(NAME2).c
	llvm-link -S ../../../tools/tracer.llvm int/$(NAME2).llvm -o int/$(NAME2)-integrated.llvm
	opt -S -instnamer -load ../../../lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME2)-integrated.llvm -o int/$(NAME2)-pass.llvm
	opt -S -instnamer -loops -load ../../../lib/libGraphGen.so -graphgen int/$(NAME2)-integrated.llvm > /dev/null
	dot -Tpng int/graphDiagram.dot -o output/graphDiagram.png
	llc -filetype=obj int/$(NAME2)-pass.llvm -o int/$(NAME2).o
	llc -filetype=asm int/$(NAME2)-pass.llvm -o int/$(NAME2).s
	$(CC) int/$(NAME2).o -o $(NAME2)-profile.out
	$(CC) -g -o $(NAME2)-test.out $(NAME2).c

server:
	$(CC) -O3 server.c -o server.out
clean: 
	rm -f $(NAME)
	rm output/*.txt
	rm int/*.dot
	rm output/*.png
	rm *.out
	rm int/*.llvm
	rm int/*.o
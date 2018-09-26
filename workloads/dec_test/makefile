CC=clang++
NAME1=compute
NAME2=supply
dectest:
	#$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o decouple.llvm decouple.h
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int1/$(NAME1).llvm $(NAME1).c
	$(CC) -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int2/$(NAME2).llvm $(NAME2).c

	llvm-link -S ../../tools/tracer.llvm int1/$(NAME1).llvm -o int1/$(NAME1)-integrated.llvm
	llvm-link -S ../../tools/tracer.llvm int2/$(NAME2).llvm -o int2/$(NAME2)-integrated.llvm

	opt -S -instnamer -load ../../lib/libRecordDynamicInfo.so -recorddynamicinfo int1/$(NAME1)-integrated.llvm -o int1/$(NAME1)-pass.llvm
	opt -S -instnamer -load ../../lib/libRecordDynamicInfo.so -recorddynamicinfo int2/$(NAME2)-integrated.llvm -o int2/$(NAME2)-pass.llvm
	opt -S -instnamer -loops -load ../../lib/libGraphGen.so -graphgen int1/$(NAME1)-integrated.llvm > /dev/null
	opt -S -instnamer -loops -load ../../lib/libGraphGen.so -graphgen int2/$(NAME2)-integrated.llvm > /dev/null

	#dot -Tpng int1/graphDiagram.dot -o output1/graphDiagram.png
	#dot -Tpng int2/graphDiagram.dot -o output2/graphDiagram.png

	llc -filetype=obj int1/$(NAME1)-pass.llvm -o int1/$(NAME1).o
	llc -filetype=obj int2/$(NAME2)-pass.llvm -o int2/$(NAME2).o

	llc -filetype=asm int1/$(NAME1)-pass.llvm -o int1/$(NAME1).s
	llc -filetype=asm int2/$(NAME2)-pass.llvm -o int2/$(NAME2).s

	$(CC) int1/$(NAME1).o -o $(NAME1)-profile.out
	$(CC) int2/$(NAME2).o -o $(NAME2)-profile.out
	$(CC) -O3 server.c -o server.out
	#./$(NAME1)-profile.out
clean: 
	rm -f $(NAME)
	rm output/*.txt
	rm int/*.dot
	rm output/*.png
	rm *.out
	rm int/*.llvm
	rm int/*.o
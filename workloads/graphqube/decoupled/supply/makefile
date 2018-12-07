CC=clang++
NAME=main
testbench: ../../main.cpp
	$(CC) -static -S -O3 -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o int/$(NAME).ll ../../$(NAME).cpp
	opt -S -load ../../../../always_inline/build/libAlwaysInlinerPass.so -alwaysinline int/$(NAME).ll -o int/$(NAME)-inlinepass.ll
	opt -S -load ../../../../compiler_passes/decouple_supply/build/libDecoupleSupplyPass.so -decouplesupply int/$(NAME)-inlinepass.ll -o int/$(NAME)-supply.ll
	#llvm-link -S ../../../../tools/tracer.llvm  int/$(NAME)-suppy.ll -o int/$(NAME)-integrated.ll
	#opt -S -instnamer -load ../../../../lib/libRecordDynamicInfo.so -recorddynamicinfo int/$(NAME)-integrated.ll -o int/$(NAME)-pass.ll
	#opt -S -instnamer -loops -load ../../../../lib/libGraphGen.so -graphgen int/$(NAME)-integrated.ll > /dev/null
	#dot -Tpng int/graphDiagram.dot -o output/graphDiagram.png
	llc -filetype=obj int/$(NAME)-supply.ll -o int/$(NAME).o
	#llc -filetype=asm int/$(NAME)-pass.ll -o int/$(NAME).s
	$(CC) ../../../../compiler_passes/stubs.o int/$(NAME).o -o $(NAME)-profile.out
	./$(NAME)-profile.out ../../_data/edges_small.tsv ../../_data/queryGraph.Subgraph.4.5.txt 20

clean: 
	rm -f $(NAME)
	rm output/*.txt
	rm int/*.dot
	rm output/*.png
	rm *.out
	rm int/*.ll
	rm int/*.o

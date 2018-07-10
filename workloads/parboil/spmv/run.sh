read name
compiler=clang
ext=c
$compiler -O1  -S -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $name.llvm $name.$ext
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -instnamer -o $name-opt.llvm $name.llvm
opt -load /home/tae/llvm/build/lib/libSlicer.so -S -scoped-noalias -enable-scoped-noalias -basicaa -scev-aa -slicer-pass -o $name-output.llvm $name-opt.llvm
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -o $name-pfinal.llvm $name-output.llvm
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -o $name-final.llvm $name-pfinal.llvm
llc -O3 -filetype=obj $name-final.llvm -o $name.o
llc -O3 -filetype=asm $name-final.llvm -o $name.s
$compiler -pthread -I/home/tae/parboil/spmv/convert-dataset -o $name.out  convert-dataset/convert_dataset.o convert-dataset/mmio.o $name.o  main.c file.c /home/tae/parboil/lib/parboil.c -lm
llc -O3 -filetype=obj $name-opt.llvm -o $name-normal.o
$compiler -pthread -I/home/tae/parboil/spmv/convert-dataset -o $name-normal.out convert-dataset/convert_dataset.o convert-dataset/mmio.o $name-normal.o main-normal.c file.c /home/tae/parboil/lib/parboil.c -lm


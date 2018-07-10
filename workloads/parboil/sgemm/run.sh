read name
compiler=clang++
ext=cc
$compiler -O1  -S -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $name.llvm $name.$ext
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -instnamer -o $name-opt.llvm $name.llvm
opt -load /home/tae/llvm/build/lib/libSlicer.so -S -scoped-noalias -enable-scoped-noalias -basicaa -scev-aa -slicer-pass -o $name-output.llvm $name-opt.llvm
#cp $name-output.llvm $name-final.llvm
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -o $name-pfinal.llvm $name-output.llvm
opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -o $name-final.llvm $name-pfinal.llvm
llc -O3 -filetype=obj $name-final.llvm -o $name.o
llc -O3 -filetype=asm $name-final.llvm -o $name.s
$compiler -pthread -o $name.out $name.o main.$ext /home/tae/parboil/lib/parboil.c io.cc -lm
#$compiler -O1  -S -fno-inline -fno-slp-vectorize -fno-vectorize -emit-llvm -o $name-normal.llvm $name-normal.$ext
#opt -O2 -disable-slp-vectorization -disable-loop-vectorization -S -instnamer -o $name-normal-opt.llvm $name-normal.llvm
llc -O3 -filetype=obj $name-opt.llvm -o $name-normal.o
$compiler -pthread -o $name-normal.out $name-normal.o main-normal.$ext /home/tae/parboil/lib/parboil.c io.cc -lm


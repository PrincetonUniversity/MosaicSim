CC=gcc
CFLAGS=-I.

all: run

run: gemm
	./gemm

gemm: gemm.o
	$(CC) -o gemm gemm.o

clean:
	rm -rf gemm *.o

PHONY: clean run

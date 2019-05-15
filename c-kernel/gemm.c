#include <stdio.h> 
#include <stdlib.h>
  
void gemm(double *inA, double *inB, double *out,
	  int rowsA, int colsA, int colsB) 
{ 
	int i, j, k; 

	for (i = 0; i < rowsA; i++) 
	{ 
		for (j = 0; j < colsB; j++) 
		{ 
			out[i * rowsA + j] = 0; 
			for (k = 0; k < colsA; k++) 
				out[i * rowsA + j] += inA[i * rowsA + k] * inB[k * colsB + j]; 
		}
	}
}
  
int main() 
{ 
	int i;
	int rowsA, colsA, colsB, batch_size;
	double **inA, **inB, **out;

	// hard-coded configuration
	rowsA = 100;
	colsA = 100;
	colsB = 100;
	batch_size = 100;

	// allocate in/out data structures
	inA = (double **) malloc(batch_size * sizeof(double*));
	inB = (double **) malloc(batch_size * sizeof(double*));
	out = (double **) malloc(batch_size * sizeof(double*));

	for (i = 0; i < batch_size; i++) {
		inA[i] = (double *) malloc(rowsA * colsA * sizeof(double));
		inB[i] = (double *) malloc(colsA * colsB * sizeof(double));
		out[i] = (double *) malloc(rowsA * colsB * sizeof(double));
	}

	// randomly fill input data structures
	// TODO: for now performing computation on uninitialized memory
	
	// iterate over the batch size
	for (i = 0; i < batch_size; i++) 
		// perform the matrix multiplication
		gemm(inA[i], inB[i], out[i], rowsA, colsA, colsB);
  
	return 0; 
}

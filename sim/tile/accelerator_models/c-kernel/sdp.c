#include <stdio.h> 
#include <stdlib.h>

// operator
// 0: '+'
// 1: '-'
// 2: '*'
// 3: '/'
// 4: ReLU
// ...

void sdp(float *inA, float *inB, float *out,
	  int lenA, int lenB, int operator) 
{ 
	int i, j, k; 

	// applies an activation functions to the elements of inA and
	// writes it to out
	if (operator > 4) // Relu, etc...
	{
	    if (operator == 5) // ReLU
		for (i = 0; i < lenA; ++i)
		    if (inA[i] < 0)
			out[i] = 0;
	}
	// element-wise operation between matrix inA and matrix inB
	else if (lenA == lenB) // matrix and matrix
	{
	    for (i = 0; i < lenA; ++i)
	    {
		if (operator == 0)
		    out[i] = inA[i] + inB[i];
		if (operator == 1)
		    out[i] = inA[i] / inB[i];
		if (operator == 2)
		    out[i] = inA[i] * inB[i];
		if (operator == 3)
		    out[i] = inA[i] / inB[i];
	    }
	}
	// element-wise operation between matrix inA and scalar inB
	else if (lenB == 1) // matrix and scalar
	{
	    for (i = 0; i < lenA; ++i)
	    {
		if (operator == 0)
		    out[i] = inA[i] + *inB;
		if (operator == 1)
		    out[i] = inA[i] / *inB;
		if (operator == 2)
		    out[i] = inA[i] * *inB;
		if (operator == 3)
		    out[i] = inA[i] / *inB;
	    }
	}
	// element-wise operation between matrix inA and vector inB
	else // matrix and vector
	{
	    j = 0;
	    for (i = 0; i < lenA; ++i)
	    {
		if (operator == 0)
		    out[i] = inA[i] + inB[j];
		if (operator == 1)
		    out[i] = inA[i] / inB[j];
		if (operator == 2)
		    out[i] = inA[i] * inB[j];
		if (operator == 3)
		    out[i] = inA[i] / inB[j];

		++j;
		if (j == lenB)
		    j = 0;
	    }
	}
}
  
int main() 
{ 
	int i;
	int rows, cols, batch_size, lenA, lenB;
	float **inA, **inB, **out;

	// example: matrix addition

	// hard-coded configuration
	rows = 100;
	cols = 100;
	batch_size = 100;
	operator = 0; // addition
	lenA = rows * cols;
	lenB = lenA;

	// allocate in/out data structures
	inA = (float **) malloc(batch_size * sizeof(float*));
	inB = (float **) malloc(batch_size * sizeof(float*));
	out = (float **) malloc(batch_size * sizeof(float*));

	for (i = 0; i < batch_size; i++) {
		inA[i] = (float *) malloc(rows * cols * sizeof(float));
		inB[i] = (float *) malloc(rows * cols * sizeof(float));
		out[i] = (float *) malloc(rows * cols * sizeof(float));
	}

	// randomly fill input data structures
	// TODO: for now performing computation on uninitialized memory
	
	// iterate over the batch size
	for (i = 0; i < batch_size; i++) 
		// perform the matrix multiplication
		sdp(inA[i], inB[i], out[i], lenA, lenB, operator);
  
	return 0; 
}

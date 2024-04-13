#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "DECADES_TensorFlow.h"

void
_kernel_(float *A, float *B, float *C, int n, int tid, int nb_threads){

  for(int i =0; i < n*n; i+=nb_threads) {
    A[i] = B[i];
    B[i] = C[i];
    C[i] = A[i];
  }

  decadesTF_matmul(n, n, n, n, 1,
  		   A, B, C, 0, 1);
}

int
main()
{
  int n = 32;
  float *A = (float *) malloc(n*n * sizeof(*A));
  float *B = (float *) malloc(n*n * sizeof(*A));
  float *C = (float *) malloc(n*n * sizeof(*A));

  for(int i =0; i < n*n; i++) {
    A[i] = 1;
    B[i] = 2;
    C[i] = 0;
  }

  _kernel_(A, B, C, n, 0, 1);

  free(A);free(B);free(C);
  return 0;
}

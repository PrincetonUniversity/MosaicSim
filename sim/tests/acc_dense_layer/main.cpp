#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "DECADES_TensorFlow.h"

void
_kernel_(float *A, float *B, float *C, int n, int tid, int nb_threads){
  for(int i =tid; i < n*n; i+=nb_threads) {
    A[i] = B[i];
    B[i] = C[i];
    C[i] = A[i];
  }

  if (!tid)
    decadesTF_dense_layer(n, n, n,  false, 0,
  			A, B, C, tid, nb_threads);
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

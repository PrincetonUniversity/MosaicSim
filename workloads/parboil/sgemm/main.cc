#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <malloc.h>
#include <vector>
#include <iostream>
#include "parboil.h"


void _kernel_sgemm( char transa, char transb, int m, int n, int k, float alpha, const float *A, int lda, const float *B, int ldb, float beta, float *C, int ldc, int tid );
struct thread_param{
	 char transa;
	 char transb;
	 int m;
	 int n;
	 int k;
	 float alpha;
	 const float *A;
	 int lda;
	 const float *B;
	 int ldb;
	 float beta;
	 float *C;
	 int ldc;
   int tid;
};

void* kernel_func(void *a_struct)
{
	struct thread_param *arg_struct = (struct thread_param *) a_struct;
	char transa = arg_struct->transa;
	char transb = arg_struct->transb;
	int m = arg_struct->m;
	int n = arg_struct->n;
	int k = arg_struct->k;
	float alpha = arg_struct->alpha;
	const float *A = arg_struct-> A;
	int lda = arg_struct->lda;
	const float *B = arg_struct-> B;
	int ldb = arg_struct->ldb;
	float beta = arg_struct->beta;
	float *C = arg_struct->C;
	int ldc = arg_struct->ldc;
	_kernel_sgemm(transa,transb,m,n,k,alpha,A,lda,B,ldb,beta,C,ldc,arg_struct->tid);

  return NULL;
}
// I/O routines
extern bool readColMajorMatrixFile(const char *fn, int &nr_row, int &nr_col, std::vector<float>&v);
extern bool writeColMajorMatrixFile(const char *fn, int, int, std::vector<float>&);

int
main (int argc, char *argv[]) {

  struct pb_Parameters *params;
  //struct pb_TimerSet timers;

  int matArow, matAcol;
  int matBrow, matBcol;
  std::vector<float> matA, matB;

  
  /* Read command line. Expect 3 inputs: A, B and B^T 
     in column-major layout*/
  params = pb_ReadParameters(&argc, argv);
  if ((params->inpFiles[0] == NULL) 
      || (params->inpFiles[1] == NULL)
      || (params->inpFiles[2] == NULL)
      || (params->inpFiles[3] != NULL))
    {
      fprintf(stderr, "Expecting three input filenames\n");
      exit(-1);
    }
 
  
  // load A
  readColMajorMatrixFile(params->inpFiles[0],
      matArow, matAcol, matA);

  // load B^T
  readColMajorMatrixFile(params->inpFiles[2],
      matBcol, matBrow, matB);

  // allocate space for C
  std::vector<float> matC(matArow*matBcol);
  struct thread_param args;
  args.transa = 'N';
  args.transb = 'T';
  args.m = matArow;
  args.n = matBcol;
  args.k = matAcol;
  args.alpha = 1.0f;
  args.A = &matA.front();
  args.lda = matArow;
  args.B = &matB.front();
  args.ldb = matBcol;
  args.beta = 0.0f;
  args.C= &matC.front();
  args.ldc = matArow;
  args.tid = 0;

  kernel_func(&args);
  if (params->outFile) {
    writeColMajorMatrixFile(params->outFile, matArow, matBcol, matC); 
  }
  pb_FreeParameters(params);
  return 0;
}

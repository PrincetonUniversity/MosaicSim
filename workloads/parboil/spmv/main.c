/***************************************************************************
 *cr
 *cr            (C) Copyright 2010 The Board of Trustees of the
 *cr                        University of Illinois
 *cr                         All Rights Reserved
 *cr
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "parboil.h"
#include "file.h"
#include "convert-dataset/convert_dataset.h"
void _kernel_spmv(int dim, float* h_data, int *h_nzcnt, int *h_ptr, int *h_indices, int *h_perm, float *h_Ax_vector, float *h_x_vector);

struct thread_param {
	 int dim;
	 float *h_data;
	 int *h_nzcnt;
	 int *h_ptr;
	 int *h_indices;
	 int *h_perm;
	 float *h_Ax_vector;
	 float *h_x_vector;
};

void* kernel_func(void *a_struct)
{
	struct thread_param *arg_struct = (struct thread_param *) a_struct;
	int dim =  arg_struct->dim;
	float *h_data = arg_struct->h_data;
	int *h_nzcnt= arg_struct->h_nzcnt;
	int *h_ptr = arg_struct->h_ptr;
	int *h_indices = arg_struct->h_indices;
	int *h_perm = arg_struct->h_perm;
	float *h_Ax_vector = arg_struct->h_Ax_vector;
	float *h_x_vector = arg_struct->h_x_vector;
	_kernel_spmv(dim,h_data,h_nzcnt,h_ptr,h_indices,h_perm,h_Ax_vector,h_x_vector);
	return NULL;
}


int main(int argc, char** argv) {
	struct pb_Parameters *parameters;
	printf("CPU-based sparse matrix vector multiplication****\n");
	printf("Original version by Li-Wen Chang <lchang20@illinois.edu> and Shengzhao Wu<wu14@illinois.edu>\n");
	printf("This version maintained by Chris Rodrigues  ***********\n");
	parameters = pb_ReadParameters(&argc, argv);
	if ((parameters->inpFiles[0] == NULL) || (parameters->inpFiles[1] == NULL))
    {
      fprintf(stderr, "Expecting two input filenames\n");
      exit(-1);
    }
	printf("Finished Reading File \n");
	//parameters declaration
	int len;
	int depth;
	int dim;
	int pad=1;
	int nzcnt_len;
	
	//host memory allocation
	//matrix
	float *h_data;
	int *h_indices;
	int *h_ptr;
	int *h_perm;
	int *h_nzcnt;
	//vector
	float *h_Ax_vector;
    float *h_x_vector;
	
	
    //load matrix from files
	//inputData(parameters->inpFiles[0], &len, &depth, &dim,&nzcnt_len,&pad,
	//    &h_data, &h_indices, &h_ptr,
	//    &h_perm, &h_nzcnt);
	int col_count;
	coo_to_jds(
		parameters->inpFiles[0], // bcsstk32.mtx, fidapm05.mtx, jgl009.mtx
		1, // row padding
		pad, // warp size
		1, // pack size
		1, // is mirrored?
		0, // binary matrix
		1, // debug level [0:2]
		&h_data, &h_ptr, &h_nzcnt, &h_indices, &h_perm,
		&col_count, &dim, &len, &nzcnt_len, &depth
	);		
	
	printf("Finished Conversion \n");

    h_Ax_vector=(float*)malloc(sizeof(float)*dim);
    h_x_vector=(float*)malloc(sizeof(float)*dim);
    input_vec( parameters->inpFiles[1], h_x_vector,dim);
	
	struct thread_param args;
	printf("Dim %d \n", dim);
	args.dim = dim;
	args.h_data = h_data;
	args.h_nzcnt = h_nzcnt;
	args.h_ptr = h_ptr;
	args.h_indices = h_indices;
	args.h_perm = h_perm;
	args.h_Ax_vector = h_Ax_vector;
	args.h_x_vector = h_x_vector;
	kernel_func(&args);
	free (h_data);
	free (h_indices);
	free (h_ptr);
	free (h_perm);
	free (h_nzcnt);
	free (h_Ax_vector);
	free (h_x_vector);
	pb_FreeParameters(parameters);

	return 0;

}

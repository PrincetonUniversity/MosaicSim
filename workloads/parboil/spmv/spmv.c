#include <stdio.h>
__attribute__((noinline)) void _kernel_spmv(int dim, float* h_data, int *h_nzcnt, int *h_ptr, int *h_indices, int *h_perm, float *h_Ax_vector, float *h_x_vector)
{	
	int p, i, k;
	int ds, de;
	//main execution // 7454500 // 1863600
	int count = 0 ;
	for(p=0;p<50;p++) // p = 50 
	{
		for (i = 0; i < dim; i=i+1) {
		  float sum = 0.0f;
		  int  bound = h_nzcnt[i];
		  count += bound;
		  for(k=0;k<bound;k++ ) {
			int j = h_ptr[k] + i;
			int in = h_indices[j];
			float d = h_data[j];
			float t = h_x_vector[in];
			sum += d*t;
		  }
		  h_Ax_vector[h_perm[i]] = sum;
		}
	}
}

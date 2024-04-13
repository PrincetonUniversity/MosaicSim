#include "DECADES.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <omp.h>

#include "../common/common_spmm.h"

void _kernel_(csc_graph A, csc_graph B, csc_graph C, float* spa, int tid, int num_threads) {
  unsigned int A_nrows = A.nodes;
  unsigned int B_ncols = B.nodes;
  float A_data, B_data;
  int num_ops = 0;

  // iterate through each column in B
  for(unsigned int j = tid; j < B_ncols; j += num_threads) {

    // iterate through each entry in the column
    for(unsigned int k = B.node_array[j]; k < B.node_array[j+1]; k++) {
      unsigned int B_idx = B.edge_array[k];
      B_data = B.edge_values[k];

      // find corresponding column in A for entry and iterate through each entry in that column
      for(unsigned int m = A.node_array[B_idx]; m < A.node_array[B_idx+1]; m++) {
        unsigned int A_idx = A.edge_array[m];
        A_data = A.edge_values[m];
        spa[j*A_nrows + A_idx] += A_data * B_data; // update cumulative sum
        num_ops += 2;
      }
    }
    
    for(unsigned int i = 0; i < A_nrows; i++) {
      unsigned int idx = j*A_nrows + i;
      if(spa[idx]) {
        C.node_array[j+1]++;
        C.edge_array[idx] = i;
        C.edge_values[idx] = spa[idx];
        spa[idx] = 0;
        //num_ops += 1;
      }
    }
  }  
  printf("num ops = %d\n", num_ops);
}

int main(int argc, char ** argv) {
 
  csr_graph A_csr = parse_csr_graph(argv[1]);
  csr_graph B_csr = parse_csr_graph(argv[2]);

  csc_graph A = convert_csr_to_csc(A_csr);
  csc_graph B = convert_csr_to_csc(B_csr);
  csc_graph C;
 
  int SPA_SIZE = A.nodes*B.nodes;
  float* spa = new float[SPA_SIZE];
  C.nodes = A.nodes; 
  C.node_array = (unsigned int*) malloc(sizeof(unsigned int)*(C.nodes+1));
  C.edge_array = (unsigned int*) malloc(sizeof(unsigned int)*SPA_SIZE);
  C.edge_values = (float*) malloc(sizeof(float)*SPA_SIZE); 

  for(unsigned int j = 0; j < SPA_SIZE; j++) {
    spa[j] = 0;
    C.edge_array[j] = 0;
    C.edge_values[j] = 0;
  }

  auto start = std::chrono::system_clock::now();

  _kernel_(A, B, C, spa, 0, 1);

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Kernel elapsed time: " << elapsed_seconds.count() << "s\n";

  return 0;
}

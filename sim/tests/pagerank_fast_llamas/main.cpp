#include "DECADES.h"
#include "DECADES_decoupled.h"
#include "dec_atomics.h"
#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

#include "graph.h"
#include "../common//common.h"

using namespace std;

void _kernel_(int compute_nodes, int compute_edges, csr_graph S, csr_graph T, float* x, float* in_r, float* out_r, int* in_wl, int* in_index, int* out_wl, int* out_index, float alpha, float epsilon, int tid, int num_threads) {
  int v, T_v, w;
  float new_r;
  int num_edges = 0;
  compute_edges /= num_threads;

  for (int i = tid; i < compute_nodes; i+=num_threads) {
    v = in_wl[i];
    x[v] = x[v] + in_r[v];
    T_v = T.node_array[v+1]-T.node_array[v];
    new_r = in_r[v]*alpha/T_v;
    int start = T.node_array[v];
    int end = T.node_array[v+1];
    if (num_edges + (end-start) > compute_edges) {
      end = compute_edges - num_edges + start;
    }
    num_edges += (end-start);
    for (int e = start; e < end; e++) {
      w = T.edge_array[e];
      float r_old = dec_atomic_fetch_add_float(&(out_r[w]), new_r);
      if ((r_old + new_r) >= epsilon && r_old < epsilon) {
        int index = compute_exclusive_fetch_add(out_index, 1);
        compute_exclusive_store((int *) out_wl + index, w);
      }
    } 
    in_r[v] = 0;
    if (num_edges == compute_edges) {
      break;
    }
  }
}

void pr(int epoch, int compute_nodes, int compute_edges, csr_graph S, csr_graph T, float* x, float* in_r, float* out_r, int* in_wl, int* in_index, int* out_wl, int* out_index, float alpha, float epsilon, int tid, int num_threads) {
  int v, T_v, w, num_epochs = 1;
  float new_r;

  while (*in_index > 0) {
    printf("-- epoch %d %d\n", num_epochs, *in_index);

    int init = tid;
    if (num_epochs == epoch) {
      if (compute_nodes == -1 || compute_nodes > *in_index) {
	compute_nodes = *in_index;
      }
      printf("----going into kernel! Computing %d nodes\n", compute_nodes);
      auto start = chrono::system_clock::now();
      _kernel_(compute_nodes, compute_edges, S, T, x, in_r, out_r, in_wl, in_index, out_wl, out_index, alpha, epsilon, tid, num_threads);
      auto end = std::chrono::system_clock::now();
      chrono::duration<float> elapsed_seconds = end-start;
      printf("----finished kernel! doing %d nodes in x86\n", *in_index - compute_nodes);
      cout << "\ndecades kernel computation time: " << elapsed_seconds.count() << "s\n";
      init = compute_nodes;
    }

    for (int i = init; i < *in_index; i+=num_threads) {
      v = in_wl[i];
      x[v] = x[v] + in_r[v];    
      T_v = T.node_array[v+1]-T.node_array[v];
      new_r = in_r[v]*alpha/T_v;
    
      for (int e = T.node_array[v]; e < T.node_array[v+1]; e++) {
        w = T.edge_array[e];
        float r_old = out_r[w];
        out_r[w] = out_r[w] + new_r;
        if (out_r[w] >= epsilon && r_old < epsilon) {
          int index = *out_index;
          *out_index = *out_index + 1;
          out_wl[index] = w;
        }
      }
      in_r[v] = 0;
    }
    
    int *tmp_wl = out_wl;
    out_wl = in_wl;
    in_wl = tmp_wl;

    float *tmp_r = out_r;
    out_r = in_r;
    in_r = tmp_r;

    *in_index = *out_index;
    *out_index = 0;
    
    num_epochs++;
  }  
}

int main(int argc, char** argv) {

  char *graph_fname;
  pr_graph graph;
  csr_graph S, T;
  float alpha = 0.85;
  float epsilon = 0.01;

  assert(argc >= 2);
  graph_fname = argv[1];

  int epoch = -1;
  if (argc >= 3) {
    epoch = atoi(argv[2]);
  }

  int compute_nodes = -1;
  int compute_edges = -1;
  if (argc >= 4) {
    compute_edges = atoi(argv[3]);
  }

  if (argc >= 5) {
    alpha = atof(argv[4]);
  }
  if (argc >= 6) {
    epsilon = atof(argv[5]);
  }

  // Create data structures
  //T = parse_csr_graph(graph_fname, false);
  T = parse_bin_files(graph_fname);

  int V = T.nodes;
  
  float * x = (float *) malloc(sizeof(float) * V);
  float * in_r = (float *) malloc(sizeof(float) * V);
  float * out_r = (float *) malloc(sizeof(float) * V);

  int * in_index = (int *) malloc(sizeof(int) * 1);
  *in_index = 0;
  int * out_index = (int *) malloc(sizeof(int) * 1);
  *out_index = 0;
  
  int * in_wl = (int *) malloc(sizeof(int) * V);
  int * out_wl = (int *) malloc(sizeof(int) * V);

  // Kernel initialization
  for (int v = 0; v < V; v++) {
    x[v] = 1 - alpha;
    in_r[v] = 0;
    out_r[v] = 0;
  }
  
  for (int v = 0; v < V; v++) {
    int T_v = T.node_array[v+1]-T.node_array[v];
    for (int i = T.node_array[v]; i < T.node_array[v+1]; i++) {
        int w = T.edge_array[i];
        in_r[w] = in_r[w] + 1.0/T_v;
    }
  
    int index = *in_index;
    in_wl[index] = v;
    *in_index = index + 1;
  }
  
  for (int v = 0; v < V; v++) {
    in_r[v] = (1-alpha)*alpha*in_r[v];
  }

  // Kernel execution
  printf("\n\nstarting kernel\n");
  auto start = chrono::system_clock::now();

  pr(epoch, compute_nodes, compute_edges, S, T, x, in_r, out_r, in_wl, in_index, out_wl, out_index, alpha, epsilon, 0 , 1);

  printf("\nending kernel");
  auto end = std::chrono::system_clock::now();
  chrono::duration<float> elapsed_seconds = end-start;
  cout << "\nkernel computation time: " << elapsed_seconds.count() << "s\n";
  
#if defined(OUTPUT_RET)
  ofstream outfile;
  outfile.open ("PR_out.txt");
  for (int v = 0; v < V; v++) {
    outfile << v << "\t" << x[v] << "\n";
  }
  outfile.close();
#endif
  
  free(x);
  free(in_r);
  free(out_r);
  free(in_index);
  free(out_index);
  free(in_wl);
  free(out_wl);
  clean_pr_graph(graph);

  return 0;
}

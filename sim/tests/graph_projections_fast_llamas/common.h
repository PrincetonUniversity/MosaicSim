#include "stdio.h"
#include "assert.h"
#include "stdlib.h"
#include <string>
#include <iostream>
#include <fstream>
#include "math.h"
#include <chrono>

#define OUTPUT_RET
//#define KERNEL_ASSERTS

using namespace std;

typedef unsigned long ulong;

// How to index into a linear upper triangle matrix
// https://stackoverflow.com/questions/27086195/linear-index-upper-triangular-matrix

ulong i_j_to_k(ulong i, ulong j, ulong n) {
  return (n*(n-1)/2) - (n-i)*((n-i)-1)/2 + j - i - 1;  
}

// returns the second term in the above to cache
ulong i_j_to_k_get_second(ulong n, ulong i) {
  return (n-i)*((n-i)-1)/2;
}

// Version of the above where the first two terms have been cached
ulong i_j_to_k_cached(ulong i, ulong j, ulong n, ulong cached, ulong cached2) {
  return cached - cached2 + j - i - 1;  
}

// Not really used, but here just in case (converts a flattened k into i,j)
void k_to_i_j(ulong k, ulong n, unsigned int *i, unsigned int *j) {
  *i = n - 2 - floor(sqrt(-8*k + 4*n*(n-1)-7)/2.0 - 0.5);
  *j = k + *i + 1 - n*(n-1)/2 + (n-*i)*((n-*i)-1)/2;  
}

// Bipartite graph
class bgraph {
public:
  unsigned int x_nodes;
  unsigned int y_nodes;
  unsigned int edges;
  unsigned int *node_array;
  unsigned int *edge_array;
  float *edge_data;
  unsigned long first_val;
};

// Parsing a bipartite graph
bgraph parse_bgraph(char *fname) {
  bgraph ret;
  ifstream reader(fname);
  string line;
  char comment;
  int first, second, third;
  float weight;

  // first line is just a comment.
  //reader >> fake;
  getline( reader, line );

  // use second line to get edges, nodes, etc.
  reader >> comment >> first >> second >> third;

  cout << "graph: " << fname << "\nedges: " << first << "\nx_graph_nodes: " << second << "\ny_graph_nodes: " << third << "\n\n";

  ret.x_nodes = second;
  ret.y_nodes = third;
  ret.edges = first;
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.x_nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));
  ret.edge_data = (float*) malloc(sizeof(float) * (ret.edges));

  int node = 0;
  ret.node_array[0] = 0;
  for(int i = 0; i < ret.edges; i++ ) {
    reader >> first >> second >> weight;
    if (first != node) {
      node++;
      ret.node_array[node] = i;
    }
    ret.edge_array[i] = second;
    ret.edge_data[i] = weight;
  }
  ret.node_array[ret.x_nodes] = ret.edges;
  ret.first_val = ((long) ret.y_nodes * ((long) ret.y_nodes - 1)/2);

  return ret;
}

void clean_bgraph(bgraph in) {
  free(in.node_array);
  free(in.edge_array);
  free(in.edge_data);
}

ulong get_projection_size_nodes(ulong n) {
  return (( n * n) - n) / 2;
}

ulong get_projection_size(bgraph &in) {
  return get_projection_size_nodes(in.y_nodes);
}

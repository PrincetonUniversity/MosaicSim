#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

/* #define OUTPUT_RET */
#define SEEDS 1

using namespace std;

// CSR graph
class csr_graph {
public:
  unsigned int nodes;
  unsigned int edges;
  unsigned int *node_array;
  unsigned int *edge_array;
  int *node_data;
};

// Parsing a bipartite graph
csr_graph parse_csr_graph(char *fname) {
  csr_graph ret;
  fstream reader(fname);
  string line;
  char comment;
  unsigned int first, second;
  float weight;

  // first line is just a comment.
  //reader >> fake;
  // getline( reader, line );

  // use second line to get edges, nodes, etc.

  auto start = chrono::system_clock::now();
  reader >> comment >> first >> second;

  cout << "graph: " << fname << "\nedges: " << first << "\ngraph_nodes: " << second << "\n\n";

  ret.nodes = second;
  ret.edges = first;
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));
  //ret.edge_data = (float*) malloc(sizeof(float) * (ret.edges));

  unsigned int node = 0;
  ret.node_array[0] = 0;
  for(unsigned int i = 0; i < ret.edges; i++ ) {
    if (i % 100000 == 0) {
      printf("reading %% %.2f finished\r", (float(i)/float(ret.edges)) * 100);
      fflush(stdout);
    }
    reader >> first >> second >> weight;
    //if (first == 232770) {
    //  printf("%d %d\n", first, second);
    //}
    if (first != node) {
      while (node != first) {
	node++;
	ret.node_array[node] = i;
      }
    }
    
    ret.edge_array[i] = second;
  }

  while (node != (ret.nodes-1)) {
    node++;
    ret.node_array[node] = ret.edges;
  }

  printf("reading %% 100.00 finished\n");
  ret.node_array[ret.nodes] = ret.edges;

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";


  return ret;
}

void clean_csr_graph(csr_graph in) {
  free(in.node_array);
  free(in.edge_array);
}


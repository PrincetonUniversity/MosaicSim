#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <climits>

/* #define OUTPUT_RET */
#define SEEDS 1

using namespace std;

typedef int weightT;
#define weight_max 1073741823

// CSR graph
class csr_graph {
public:
  unsigned int nodes;
  unsigned int edges;
  unsigned int *node_array;
  unsigned int *edge_array;
  weightT *edge_values;
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
  ret.edge_values = (weightT*) malloc(sizeof(weightT) * (ret.edges));
  //ret.edge_data = (float*) malloc(sizeof(float) * (ret.edges));

  unsigned int node = 0;
  ret.node_array[0] = 0;
  float third = 0;
  for(unsigned int i = 0; i < ret.edges; i++ ) {
    if (i % 100000 == 0) {
      printf("reading %% %.2f finished\r", (float(i)/float(ret.edges)) * 100);
      fflush(stdout);
    }
    reader >> first >> second >> third;
    if (first != node) {
      while (node != first) {
	node++;
	ret.node_array[node] = i;
      }
    }
    ret.edge_array[i] = second;
    ret.edge_values[i] = (weightT) third;
  }
  printf("reading %% 100.00 finished\n");
  ret.node_array[ret.nodes] = ret.edges;

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";


  return ret;
}

csr_graph parse_bin_files(string base) {
  csr_graph ret;
  ifstream nodes_edges_file(base + "num_nodes_edges.txt");
  int nodes, edges;
  auto start = chrono::system_clock::now();
  
  nodes_edges_file >> nodes;
  nodes_edges_file >> edges;
  nodes_edges_file.close();
  cout << "found " << nodes << " " << edges << "\n";
  
  ret.nodes = nodes;
  ret.edges = edges;
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));
  ret.edge_values = (weightT*) malloc(sizeof(weightT) * (ret.edges));

  ifstream node_array_file;
  node_array_file.open(base + "node_array.bin", ios::in | ios::binary);
  
  if (!node_array_file.is_open()) {
    cout << "no node array file" << endl;
    assert(0);
  }

  node_array_file.seekg (0, node_array_file.end);
  long int length = node_array_file.tellg();
  node_array_file.seekg (0, node_array_file.beg);
  cout << "byte length of node array: " << length << endl;

  cout << "reading byte length of:    " << (ret.nodes + 1) * sizeof(unsigned int) << endl;
  node_array_file.read((char *)ret.node_array, (ret.nodes + 1) * sizeof(unsigned int));
  node_array_file.close();
  ret.node_array[0] = 0;

  ifstream edge_array_file;
  edge_array_file.open(base + "edge_array.bin", ios::in | ios::binary);
  
  if (!edge_array_file.is_open()) {
    cout << "no edge array file" << endl;
    assert(0);
  }
  
  edge_array_file.seekg (0, edge_array_file.end);
  length = edge_array_file.tellg();
  edge_array_file.seekg (0, edge_array_file.beg);
  cout << "byte length of edge array: " << length << endl;

  cout << "reading byte length of:    " << (ret.edges) * sizeof(unsigned int) << endl;
  edge_array_file.read((char*)ret.edge_array, (ret.edges) * sizeof(unsigned int));
  edge_array_file.close();

  ifstream edge_values_file;
  edge_values_file.open(base + "edge_values.bin", ios::in | ios::binary);
  if (!edge_values_file.is_open()) {
    cout << "no edge values file" << endl;
    assert(0);
  }

  edge_values_file.read((char*)ret.edge_values, sizeof(weightT) * ret.edges); 
  edge_values_file.close();        

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";

  return ret;  
}

void clean_csr_graph(csr_graph in) {
  free(in.node_array);
  free(in.edge_array);
}

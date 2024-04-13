#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <climits>

#define OUTPUT_RET
#define SEEDS 1

using namespace std;

typedef float weightT;
#define weight_max 1073741823

// CSR graph
class csr_graph {
public:
  unsigned int nodes;
  unsigned int edges;
  unsigned int *node_array;
  unsigned int *edge_array;
  weightT *edge_values;
};

class csc_graph {
public:
  unsigned int nodes;
  unsigned int edges;
  unsigned int *node_array;
  unsigned int *edge_array;
  weightT *edge_values;
};

csc_graph convert_csr_to_csc(csr_graph G) {
  csc_graph ret;
  auto start = chrono::system_clock::now();

  printf("converting csr to csc\n");
  
  ret.nodes = G.nodes;
  ret.edges = G.edges;
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));
  ret.edge_values = (weightT*) malloc(sizeof(weightT) * (ret.edges));
  unsigned int * incoming_edges = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes));
  
  for (unsigned int i = 0; i < ret.nodes; i++) {
    incoming_edges[i] = 0;
  }
  for (unsigned int i = 0; i < ret.edges; i++) {
    ret.edge_array[i] = -1;
  }

  // count incoming edges
  unsigned int total_edges = 0;
  unsigned int duplicates = 0;
  for (unsigned int node = 0; node < ret.nodes; node++) {
    const unsigned int start = G.node_array[node];
    const unsigned int end = G.node_array[node+1];
    assert(start <= end); // check number of neighbors
    int previous = -1;
    for (unsigned int e = start; e < end; e++) {
      assert(e <= G.edges); // check edge number
      const unsigned int edge_index = G.edge_array[e];
      assert(edge_index < ret.nodes); // check edge index
      assert((int)edge_index >= previous);
      
      incoming_edges[edge_index]++;
      total_edges++;
      if (edge_index == previous) duplicates++;
      previous = edge_index;
    }
  }
  cout << "total edges: " << total_edges <<", duplicates: " << duplicates << endl;

  // prefix sum
  for (unsigned int n = 1; n < ret.nodes; n++) {
    incoming_edges[n] += incoming_edges[n-1];
  }
  assert(incoming_edges[ret.nodes-1] == ret.edges);

  // finish up the return array
  ret.node_array[0] = 0;
  for (unsigned int n = 1; n <= ret.nodes; n++) {
    ret.node_array[n] = incoming_edges[n-1];
  }

  // repurpose incoming edges as an offset in the edge array
  for (unsigned int i = 0; i < ret.nodes; i++) {
    incoming_edges[i] = 0;
  }

  for (unsigned int n = 0; n < ret.nodes; n++) {
    const unsigned int node = n;
    const unsigned int start = G.node_array[node];
    const unsigned int end = G.node_array[node+1];
    for (unsigned int e = start; e < end; e++) {
      const unsigned int edge_index = G.edge_array[e];
      const float data = G.edge_values[e];
      unsigned int base_index = ret.node_array[edge_index];
      unsigned int base_offset = incoming_edges[edge_index];
      ret.edge_array[base_index + base_offset] = n;
      ret.edge_values[base_index + base_offset] = data;
      incoming_edges[edge_index]++;
    }
  }

  for (unsigned int e = 0; e < ret.edges; e++) {
    assert(ret.edge_array[e] != -1);
  }

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Converting graph elapsed time: " << elapsed_seconds.count() << "s\n\n";

  free(incoming_edges);
  return ret;
}

csr_graph parse_csr_graph(char *fname) {
  csr_graph ret;
  ifstream reader(fname);
  string line;
  char comment;
  unsigned int first, second;
  float weight;

  auto start = chrono::system_clock::now();
  reader >> comment >> first >> second;

  cout << "graph: " << fname << "\nedges: " << first << "\ngraph_nodes: " << second << "\n\n";

  ret.nodes = second;
  ret.edges = first;
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));
  ret.edge_values = (weightT*) malloc(sizeof(weightT) * (ret.edges));

  unsigned int node = 0;
  ret.node_array[0] = 0;
  float third = 0;
  unsigned int i;
  for(i = 0; i < ret.edges; i++ ) {
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
  
  while (node != (ret.nodes-1)) {
    node++;
    ret.node_array[node] = i;
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

csr_graph parse_bin_files(string base) {
  csr_graph ret;
  ifstream nodes_edges_file(base + "num_nodes_edges.txt");
  unsigned int nodes, edges;
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

  edge_values_file.read((char*)ret.edge_values, sizeof(int) * ret.edges); 
  edge_values_file.close();        

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";

  return ret;  
}

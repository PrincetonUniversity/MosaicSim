#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "assert.h"
#include <iostream>
#include <string>
#include <fstream>
#include <chrono>

#define OUTPUT_RET
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

// PR graph
class pr_graph {
public:
  csr_graph S;
  csr_graph T;
};

// Parsing a bipartite graph
csr_graph parse_csr_graph(char *fname, bool make_S) {
  csr_graph ret;
  fstream reader(fname);
  ofstream S_file;
  string line;
  char comment;
  unsigned int first, second;
  float third;

  auto start = chrono::system_clock::now();
  
  reader >> comment >> first >> second;
  ret.nodes = second;
  ret.edges = first;
 
  cout << ret.nodes << " " << ret.edges << endl; 
  if (make_S) {
    string S_filename = "S.tsv";
    S_file.open(S_filename);
    S_file << "c " << ret.edges << " " << ret.nodes << "\n";
  }

  cout << "graph: " << fname << "\nedges: " << ret.edges << "\ngraph_nodes: " << ret.nodes << "\n\n";
  ret.node_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.nodes + 1));
  ret.edge_array = (unsigned int*) malloc(sizeof(unsigned int) * (ret.edges));

  unsigned int node = 0;
  ret.node_array[0] = 0;
  for(unsigned int i = 0; i < ret.edges; i++ ) {
    if (i % 100000 == 0) {
      printf("reading %% %.2f finished\r", (float(i)/float(ret.edges)) * 100);
      fflush(stdout);
    }
    reader >> first >> second >> third;

    if (make_S) {
      S_file << second << " " << first << " 1" << "\n";
    }
    
    if (first != node) {
      while (node != first) {
	node++;
	ret.node_array[node] = i;
      }
    }
    ret.edge_array[i] = second;
  }

  printf("reading %% 100.00 finished\n");
  ret.node_array[ret.nodes] = ret.edges;

  auto end = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = end-start;
  cout << "Reading graph elapsed time: " << elapsed_seconds.count() << "s\n";

  if (make_S) {
    S_file.close();
  }

  return ret;
}

pr_graph create_pr_graph(char *fname) {
  pr_graph graph;
  csr_graph S, T;

  cout << "Reading T!\n**********\n";

  T = parse_csr_graph(fname, true);
  system("(head -n 1 S.tsv && tail -n +2 S.tsv | sort -k1n,1 -k2n,2) > S_edge_list.tsv");

  cout << "\nReading S!\n**********\n";
  S = parse_csr_graph((char*)"S_edge_list.tsv", false);
  //S = parse_csr_graph((char*)"S_edge_list.tsv", false);

  //system("rm S.tsv");
  //system("rm S_edge_list.tsv");
 
  graph.S = S;
  graph.T = T;
  
  return graph;
}

void clean_csr_graph(csr_graph in) {
  free(in.node_array);
  free(in.edge_array);
}

void clean_pr_graph(pr_graph in) {
  clean_csr_graph(in.S);
  clean_csr_graph(in.T);
}

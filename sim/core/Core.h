#ifndef CORE_H
#define CORE_H

#include "../common.h"
#include "../graph/Graph.h"
#include "LoadStoreQ.h"
#include <string>
#include <chrono>
using namespace std;

class DyanmicNode;
class Graph;
class Interconnect;
class Digestor;
class Context;
class DRAMSimInterface;
class Cache;


typedef chrono::high_resolution_clock Clock;

class Core 
{
public:
  string name; 
  Graph g;
  uint64_t cycles = 0;
  DRAMSimInterface* memInterface;
  Interconnect* intercon;
  queue<DynamicNode*> inputQ;
  Cache* cache;
  Digestor* digestor;
  Statistics local_stat;
  bool has_digestor=false;
  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;
  int context_to_create = 0;

  /* Resources / limits */
  map<TInstr, int> available_FUs;
  map<BasicBlock*, int> outstanding_contexts;
    
  /* Activity counters */
  map<TInstr, int> activity_FUs;
  struct {
    int bytes_read;
    int bytes_write;
  } activity_mem;

  /* Profiled */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /* LSQ */
  LoadStoreQ lsq;
  void initialize();
  bool createContext();
  bool process_cycle();
  void process_memory();
  void run();
  void toMemHierarchy(DynamicNode *d);
  void printActivity();
};
#endif
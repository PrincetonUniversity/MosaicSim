#ifndef CORE_H
#define CORE_H

#include "../common.h"
#include "../sim.h"
#include "../graph/Graph.h"
#include "LoadStoreQ.h"
#include <string>
#include <chrono>
using namespace std;

class DyanmicNode;
class Graph;
class Interconnect;
class Simulator;
class Context;
class DRAMSimInterface;
class Cache;

typedef chrono::high_resolution_clock Clock;

class Core {
public:
  string name; 
  int id;
  Graph g;
  uint64_t cycles = 0;
  Interconnect* intercon;
  //queue<DynamicNode*> inputQ;

  Simulator* master;
  Statistics local_stat;

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
  
  unordered_map<uint64_t, DynamicNode*> access_tracker;
  queue<int> tracker_id;
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /* LSQ */
  LoadStoreQ lsq;
  void initialize(int id);
  bool createContext();
  bool process();
  bool canAccess(bool isLoad);
  void access(DynamicNode *d);
  void accessComplete(Transaction *t);
  void printActivity();
};
#endif

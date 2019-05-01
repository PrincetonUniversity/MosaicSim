#ifndef CORE_H
#define CORE_H
#include "../common.h"
#include "../sim.h"
#include "Tile.h"
#include "../graph/Graph.h"
#include "LoadStoreQ.h"
#include <string>
#include <chrono>
using namespace std;

class DyanmicNode;
class Graph;
class Simulator;
class Context;
class DRAMSimInterface;
class Cache;

typedef chrono::high_resolution_clock Clock;

struct DNPointerLT {
    bool operator()(const DynamicNode* a, const DynamicNode* b) const {
        return *a < *b;
    }
};

class IssueWindow {
public:
  map<DynamicNode*, uint64_t, DNPointerLT> issueMap;
  vector<DynamicNode*> issueQ;
  int window_size=1;//128; // instrn window size
  uint64_t window_start=0;
  uint64_t curr_index=0;
  uint64_t window_end=window_size-1;
  int issueWidth=1;//8; //total # issues per cycle
  int issueCount=0;
  uint64_t cycles=0;
  void insertDN(DynamicNode* d);
  bool canIssue(DynamicNode* d);
  void process();
  void issue();
};

class Core: public Tile {
public:
  string name; 
  Graph g;
  //uint64_t cycles = 0;
  //queue<DynamicNode*> inputQ;
  IssueWindow window;
  Config local_cfg; 
  //Simulator* sim;
  Cache* cache;
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

  /* Dynamic Traces */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  unordered_map<uint64_t, DynamicNode*> access_tracker;
  queue<int> tracker_id;
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /* LSQ */
  LoadStoreQ lsq=LoadStoreQ(false);

  Core(Simulator* sim, int clockspeed);
  
  bool ReceiveTransaction(Transaction* t);
  void initialize(int id);
  bool createContext();
  bool process();
  bool canAccess(bool isLoad);
  void access(DynamicNode *d);
  bool communicate(DynamicNode *d);
  void accessComplete(MemTransaction *t);
  void printActivity();
  bool predict_branch(DynamicNode* d);
  string instrToStr(TInstr instr);
};
#endif

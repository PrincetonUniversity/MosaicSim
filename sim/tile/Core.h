#ifndef CORE_H
#define CORE_H

#include "../sim.h"
#include "Bpred.h"
#include "LoadStoreQ.h"

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
  vector<DynamicNode*> barrierVec;

  int window_size=1;//128; // instrn window size
  uint64_t window_start=0;
  uint64_t curr_index=0;
  uint64_t window_end=window_size-1;
  int issueWidth=1;//8; //total # issues per cycle
  int issueCount=0;
  uint64_t cycles=0;
  void insertDN(DynamicNode* d);
  bool canIssue(DynamicNode* d);
  void issue(DynamicNode* d);
  void process();
};


class Core: public Tile {
public:
  Graph g;
  IssueWindow window;
  bool windowFull=false;
  Config local_cfg;
  Cache* cache;
  Cache* l2_cache;
  Bpred *bpred;
  Statistics local_stat;

  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;
  int context_to_create = 0;
  uint64_t total_created_contexts = 0;

  /* Resources */
  unordered_map<int, int> available_FUs;
  unordered_map<BasicBlock*, int> outstanding_contexts;
  double total_energy = 0.0;
  double avg_power = 0.0;

  /* Dynamic Traces */
  vector<int> cf; // List of basic blocks in "sequential" program order
  map<int, pair<bool,set<int>>> bb_cond_destinations; // map of basic blocks indicating if "conditional" and "destinations"
  ifstream memfile;
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  unordered_map<int, queue<string> > acc_map;  // List of memory accesses per instruction in a program order

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
  void deleteErasableContexts();
  void calculateEnergyPower();
  bool predict_branch_and_check(DynamicNode* d);
  string getInstrName(TInstr instr);
  void fastForward(uint64_t inc);
};
#endif

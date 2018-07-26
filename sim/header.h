#ifndef HEADER_H
#define HEADER_H

#include "include/DRAMSim.h"
#include "functional_cache.h"
#include "graphOpt.h"

class Simulator;
class DynamicNode;
class Context;
class DRAMSimInterface;
class Config;
class Statistics;
using namespace std;

class Statistics {
public:
  map<string, pair<int, int>> stats;
  int num_types = 4;
  void registerStat(string str, int type) {
    stats.insert(make_pair(str, make_pair(0, type)));
  }
  Statistics() {
    registerStat("cycles", 0);
    registerStat("total_instructions", 0);
    registerStat("contexts", 0);

    registerStat("cache_hit", 1);
    registerStat("cache_miss", 1);
    registerStat("cache_access", 1);
    registerStat("cache_pending", 1);
    registerStat("cache_evict", 1);
    registerStat("dram_access", 1);

    registerStat("speculated", 2);
    registerStat("misspeculated", 2);
    registerStat("forwarded", 2);
    registerStat("speculatively_forwarded", 2);

    registerStat("load_issue_try", 3);
    registerStat("load_issue_success", 3);
    registerStat("store_issue_try", 3);
    registerStat("store_issue_success", 3);
    registerStat("comp_issue_try", 3);
    registerStat("comp_issue_success", 3);

    registerStat("lsq_insert_success", 4);
    registerStat("lsq_insert_fail", 4);
  }
  int get(string str) {
    return stats.at(str).first;
  }
  void set(string str, int set) {
    stats.at(str).first = set;
  }
  void update(string str, int inc=1) {
    stats.at(str).first += inc;
  }
  void reset() {
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      it->second.first = 0;
    }
  }
  void print() {
    cout << "IPC : " << (double) get("total_instructions") / get("cycles") << "\n";
    cout << "BW : " << (double) get("dram_access") * 64 / get ("cycles") << " GB/s \n";
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      cout << it->first << " : " << it->second.first << "\n";
    }
  }
};

extern Statistics stat;

typedef pair<DynamicNode*, uint64_t> Operator;

class DynamicNode {
public:
  Node *n;
  Context *c;
  Simulator *sim;
  TInstr type;
  bool issued = false;
  bool completed = false;
  bool isMem;
  /* Memory */
  uint64_t addr;
  bool addr_resolved = false;
  bool speculated = false;
  int outstanding_accesses = 0;
  /* Depedency */
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;

  DynamicNode(Node *n, Context *c, Simulator *sim, uint64_t addr = 0);
  bool operator== (const DynamicNode &in);
  bool operator< (const DynamicNode &in) const;
  void print(string str, int level = 0);
  void handleMemoryReturn();
  void tryActivate(); 
  bool issueMemNode();
  bool issueCompNode();
  void finishNode();
};

struct DynamicNodePointerCompare {
  bool operator() (const DynamicNode* a, const DynamicNode* b) const {
     return *a < *b;
  }
};

struct OpCompare {
  bool operator() (const Operator &l, const Operator &r) const {
    if(r.second < l.second) 
      return true;
    else if (l.second == r.second)
      return (*(r.first) < *(l.first));
    else
      return false;
  }
};

class Context {
public:
  bool live;
  unsigned int id;
  
  Simulator* sim;
  BasicBlock *bb;

  int next_bbid;
  int prev_bbid;

  std::map<Node*, DynamicNode*> nodes;
  std::set<DynamicNode*, DynamicNodePointerCompare> issue_set;
  std::set<DynamicNode*, DynamicNodePointerCompare> speculated_set;
  std::set<DynamicNode*, DynamicNodePointerCompare> next_issue_set;
  std::set<DynamicNode*, DynamicNodePointerCompare> completed_nodes;
  std::set<DynamicNode*, DynamicNodePointerCompare> nodes_to_complete;

  priority_queue<Operator, vector<Operator>, OpCompare> pq;

  Context(int id, Simulator* sim) : live(false), id(id), sim(sim) {}
  Context* getNextContext();
  Context* getPrevContext();
  void insertQ(DynamicNode *d);
  void print(string str, int level = 0);
  void initialize(BasicBlock *bb, int next_bbid, int prev_bbid);
  void process();
  void complete();
};

class LoadStoreQ {
public:
  deque<DynamicNode*> lq;
  deque<DynamicNode*> sq;
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> lm;
  unordered_map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> sm;
  set<DynamicNode*, DynamicNodePointerCompare> us;
  set<DynamicNode*, DynamicNodePointerCompare> ul;

  int size;
  bool set_processed = false;
  
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_st_set;
  set<DynamicNode*,DynamicNodePointerCompare> unresolved_ld_set;


  LoadStoreQ();
  void resolveAddress(DynamicNode *d);
  void insert(DynamicNode *d);
  bool checkSize(int num_ld, int num_st);
  bool check_unresolved_load(DynamicNode *in);
  bool check_unresolved_store(DynamicNode *in);
  int check_load_issue(DynamicNode *in, bool speculation_enabled);
  bool check_store_issue(DynamicNode *in);
  int check_forwarding (DynamicNode* in);
  std::vector<DynamicNode*> check_speculation (DynamicNode* in);
};

class Cache {
public:
  DRAMSimInterface *memInterface;
  vector<DynamicNode*> to_send;
  vector<uint64_t> to_evict;
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  
  uint64_t cycles = 0;
  int latency;
  int size_of_cacheline;
  bool ideal;
  
  FunctionalCache *fc;
  Cache(int latency, int size, int assoc, int linesize, bool ideal): 
    latency(latency), size_of_cacheline(linesize), ideal(ideal), fc(new FunctionalCache(size, assoc)) {}
  bool process_cache();
  void execute(DynamicNode* d);
  void addTransaction(DynamicNode *d);
};

class DRAMSimInterface {
public:
  Cache *c;
  bool ideal;
  int mem_load_ports;
  int mem_store_ports;
  unordered_map< uint64_t, queue<DynamicNode*> > outstanding_read_map;
  DRAMSim::MultiChannelMemorySystem *mem;

  DRAMSimInterface(Cache *c, bool ideal, int mem_load_ports, int mem_store_ports) : c(c),ideal(ideal), mem_load_ports(mem_load_ports), mem_store_ports(mem_store_ports) {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(DynamicNode* d, uint64_t addr, bool isLoad);
  bool willAcceptTransaction(DynamicNode* d, uint64_t addr);
};

class Simulator 
{
public:
  int issue_count=0; //luwa delete
  Graph g;
  uint64_t cycles = 0;
  DRAMSimInterface* memInterface; 
  Cache* cache;
  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;
  int context_to_create = 0;

  /* Resources / limits */
  map<TInstr, int> available_FUs;
  map<BasicBlock*, int> outstanding_contexts;
  int ports[4]; // ports[0] = cache loads; ports[1] = cache stores; //ports[2] = mem loads; ports[3] = mem stores;
  
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

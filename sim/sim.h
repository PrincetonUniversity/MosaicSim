//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "base.h"
#include "include/DRAMSim.h"
#include "functional_cache.h"

using namespace apollo;
using namespace std;

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
class Simulator;



class GlobalStats {
public:
  // some global stats
  int num_exec_instr;
  int num_cycles;
  int num_finished_context;
  int num_L1_hits;
  int num_L1_misses;
  
  GlobalStats() { reset(); }

  void reset() {
    num_exec_instr = 0;
    num_cycles = 0;
    num_finished_context = 0;
    num_L1_hits = 0;
    num_L1_misses = 0;
  }
  void print() {
    cout << "** Global Stats **\n";
    cout << "num_exec_instr = " << num_exec_instr << endl;
    cout << "num_cycles     = " << num_cycles << endl;
    cout << "IPC            = " << num_exec_instr / (double)num_cycles << endl;
    cout << "num_finished_context = " << num_finished_context << endl;
    cout << "L1_hits = " << num_L1_hits << endl;
    cout << "L1_misses = " << num_L1_misses << endl;
    cout << "L1_hit_rate = " << num_L1_hits / (double)(num_L1_hits+num_L1_misses) << endl;
  }
};

class DynamicNode;
typedef pair<DynamicNode*, uint64_t> Operator;

struct OpCompare {
  friend bool operator< (const Operator &l, const Operator &r) {
    if(l.second < r.second) 
      return true;
    else if(l.second == r.second)
      return true;
    else
      return false;
  }
};

class Context {
public:
  bool live;
  int id;

  Simulator* sim;
  Config *cfg;
  BasicBlock *bb;

  int next_bbid;
  int prev_bbid;

  std::map<Node*, DynamicNode*> nodes;
  //std::vector<DynamicNode*> active_list;
  std::set<DynamicNode*> issue_set;
  std::set<DynamicNode*> waiting_set;
  std::set<DynamicNode*> next_issue_set;
  //std::vector<DynamicNode*> next_active_list;
  std::vector<DynamicNode*> nodes_to_complete;
  std::set<DynamicNode*> completed_nodes;
  typedef pair<DynamicNode*, uint64_t> Op;
  priority_queue<Operator, vector<Operator>, less<vector<Operator>::value_type> > pq;

  Context(int id, Simulator* sim) : live(false), id(id), sim(sim) {}
  Context* getNextContext();
  Context* getPrevContext();
  void insertQ(DynamicNode *d);
  void process();
  void complete();
  void initialize(BasicBlock *bb, Config *cfg, int next_bbid, int prev_bbid);
};

class DynamicNode {
public:
  Node *n;
  Context *c;
  Simulator *sim;
  Config *cfg;
  TInstr type;
  bool issued = false;
  bool completed = false;
  bool isMem;
  /* Memory */
  uint64_t addr;
  bool addr_resolved;
  bool speculated = false;
  int outstanding_accesses = 0;
  /* Depedency */
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;

  DynamicNode(Node *n, Context *c, Simulator *sim, Config *cfg, uint64_t addr = 0): n(n), c(c), sim(sim), cfg(cfg), addr(addr) {
    type = n->typeInstr;
    if(type == PHI) {
      bool found = false;
      for(auto it = n->phi_parents.begin(); it!= n->phi_parents.end(); ++it) {
        if((*it)->bbid == c->prev_bbid) {
          found = true;
          break;
        }
      }
      if(found)
        pending_parents = 1;
      else
        pending_parents = 0;
    }
    else
      pending_parents = n->parents.size();
    pending_external_parents = n->external_parents.size();
    if(addr == 0)
      isMem = false;
    else
      isMem = true;
  }
  bool operator== (const DynamicNode &in) {
    if(c->id == in.c->id && n->id == in.n->id)
      return true;
    else
      return false;
  }
  bool operator< (const DynamicNode &in) const {
    if(c->id < in.c->id) 
      return true;
    else if(c->id == in.c->id && n->id < in.n->id)
      return true;
    else
      return false;
  }
  bool issueMemNode();
  bool issueCompNode();
  void finishNode();
  void tryActivate(); 
  void handleMemoryReturn();
  
  friend std::ostream& operator<<(std::ostream &os, DynamicNode &d) {
    os << "[Context-" <<d.c->id <<"] Node" << d.n->name <<" ";
    return os;
  }
  void print(string str, int level = 0) {
    if( level < cfg->vInputLevel )
      cout << (*this) << str << "\n";
  }

};

class LoadStoreQ {
public:
  deque<DynamicNode*> q;
  int size;
  void insert(DynamicNode *d) {
    q.push_back(d);
  }
  bool checkSize(int requested_space) {
    if(q.size() <= size - requested_space) {
      return true;
    }
    else {
      int ct = 0;
      for(deque<DynamicNode*>::iterator it = q.begin(); it!= q.end(); ++it) {
        if((*it)->completed)
          ct++;
        else
          break;
      }
      if(ct > requested_space) {
        for(int i=0; i<requested_space; i++)
          q.pop_front();
        return true;
      }
      return false;
    }
  }
  //there's at least one unstarted (unknown address) older memop
  bool exists_unresolved_memop (DynamicNode* in, TInstr op_type) {
    for(deque<DynamicNode*>::iterator it = q.begin(); it!= q.end(); ++it) {
      DynamicNode *d = *it;
      if(*d == *in)
        return false;
      else if(*in < *d) { // input is older (lower cid) than the current one
        if(in->c->id > d->c->id)
          assert(false);
        if(in->c->id ==  d->c->id && in->n->id > d->n->id)
          assert(false);
        if(in->c->id == d->c->id && in->n->id == d->n->id)
          assert(false);
        return false;
      }
      if(!(d->addr_resolved) && d->type == op_type) {
        return true;
      }     
    }
    return false;
  }
  //there's at least one addr_resolved, uncompleted older memop with same address
  //negation says for all memops with addr_resolved, uncompleted older memops don't match address
  bool exists_conflicting_memop (DynamicNode* in, TInstr op_type) {
    for(deque<DynamicNode*>::iterator it = q.begin(); it!= q.end(); ++it) {
      DynamicNode *d = *it;
      if(*d == *in)
        return false;
      else if(*in < *d)
        return false;
      if((d->addr_resolved) && !(d->completed) && d->n->typeInstr == op_type && d->addr == in->addr)
        return true;
    }
    return false;
  }
  int check_forwarding (DynamicNode* in) {
    bool found = false;
    bool speculative = false;
    for(deque<DynamicNode*>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      DynamicNode *d = *it;
      if(*d == *in) {
        found = true;
        continue;
      }
      if(found) {
        if(d->addr == in->addr)
          if(d->completed && !speculative)
            return 1;
          else if(d->completed && speculative)
            return 0;
          else
            return -1;
        else if(!d->addr_resolved) {
          speculative = true;
        }
      }
    }
    return -1;
  }
  std::vector<DynamicNode*> check_speculation (DynamicNode* in) {
    std::vector<DynamicNode*> ret;
    for(deque<DynamicNode*>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      DynamicNode *d = *it;
      if(*d == *in)
        break;
      else if(*d < *in)
        break;
      if(d->speculated && !(d->completed) && d->n->typeInstr == LD && d->addr == in->addr) {
        d->speculated = false;
        ret.push_back(d);
      }
    }
    return ret;
  }
};


class Cache;
class DRAMSimInterface {
public:
  map< uint64_t, queue<DynamicNode*> > outstanding_load_map;
  map< uint64_t, queue<DynamicNode*> > outstanding_store_map;;
  DRAMSim::MultiChannelMemorySystem *mem;
  Cache *c;
  bool ideal;
  DRAMSimInterface(Cache *c, bool ideal) : c(c),ideal(ideal)  {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t mem_clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(DynamicNode* d, uint64_t addr, bool isLoad);
};
class Cache {
public:
  typedef pair<DynamicNode*, uint64_t> Operator;
  uint64_t cycles = 0;
  DRAMSimInterface *memInterface;
  int size_of_cacheline = 64;
  int latency;
  FunctionalSetCache *fc;
  bool ideal=false;
  priority_queue<Operator, vector<Operator>, less<vector<Operator>::value_type> > pq;
  vector<DynamicNode*> to_send;
  vector<uint64_t> to_evict;

  GlobalStats *stats;
  
  Cache(int latency, int size, int assoc, int block_size, bool ideal, DRAMSimInterface *memInterface, GlobalStats *stats): 
            latency(latency), ideal(ideal), memInterface(memInterface), stats(stats) {
    fc = new FunctionalSetCache(size, assoc, block_size);  
  }
  
  void process_cache() {
    while(pq.size() > 0) {
      if(pq.top().second > cycles)
        break;
      execute(pq.top().first);
      pq.pop();
    }
    for(auto it = to_send.begin(); it!= to_send.end();) {
      DynamicNode *d = *it;
      uint64_t dramaddr = d->addr/size_of_cacheline * size_of_cacheline;
      if(memInterface->mem->willAcceptTransaction(dramaddr)) {
        memInterface->addTransaction(d, dramaddr, d->type == LD);
        it = to_send.erase(it);
      }
      else
        it++;
    }
    for(auto it = to_evict.begin(); it!= to_evict.end();) {
      uint64_t eAddr = *it;
      if(memInterface->mem->willAcceptTransaction(eAddr)) {
        memInterface->addTransaction(NULL, eAddr, false);
        it = to_evict.erase(it);
      }
      else
        it++;
    }
  }
  void execute(DynamicNode* d) {
    uint64_t dramaddr = d->addr/size_of_cacheline * size_of_cacheline;
    bool res = true;
    if(!ideal)
      res = fc->access(dramaddr/size_of_cacheline);
    if (res) {                  
      d->handleMemoryReturn();
      d->print("Hits in Cache", 2);
      stats->num_L1_hits++;
    }
    else {
      to_send.push_back(d);
      d->print("Misses in Cache", 2);
      stats->num_L1_misses++;
    }
  }
  
  void addTransaction(DynamicNode *d) {
    d->print("Added Cache Transaction", 2);
    pq.push(make_pair(d, cycles+latency));   
  }   
};
void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t mem_clock_cycle) {
  assert(outstanding_load_map.find(addr) != outstanding_load_map.end());
  queue<DynamicNode*> &q = outstanding_load_map.at(addr);
  while(q.size() > 0) {
    DynamicNode* d = q.front();  
    d->handleMemoryReturn();
    q.pop();
  }
  int64_t evictedAddr = -1;
  if(!ideal)
    c->fc->insert(addr/64, &evictedAddr);
  if(evictedAddr!=-1) {
    assert(evictedAddr >= 0);
    c->to_evict.push_back(evictedAddr*64);
  }
  if(q.size() == 0)
    outstanding_load_map.erase(addr);
}
void DRAMSimInterface::write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  if(outstanding_store_map.find(addr) != outstanding_store_map.end()) { 
    queue<DynamicNode*> &q = outstanding_store_map.at(addr);
    DynamicNode* d = q.front();
    d->handleMemoryReturn();
    q.pop();
    if(q.size() == 0)
      outstanding_store_map.erase(addr);
  }
}
void DRAMSimInterface::addTransaction(DynamicNode* d, uint64_t addr, bool isLoad) {
  if(d!= NULL && d->type == LD) {
    if(outstanding_load_map.find(addr) == outstanding_load_map.end()) {
      outstanding_load_map.insert(make_pair(addr, queue<DynamicNode*>()));
      mem->addTransaction(!isLoad, addr);
    }
    outstanding_load_map.at(addr).push(d);  
  }
  else if (d!= NULL && d->type == ST) {
    if(outstanding_store_map.find(addr) == outstanding_store_map.end()) {
      outstanding_store_map.insert(make_pair(addr, queue<DynamicNode*>()));
      mem->addTransaction(!isLoad, addr);
    }
    outstanding_store_map.at(addr).push(d);  
  }
  else {
    mem->addTransaction(!isLoad, addr);
  }
}

class Simulator 
{
public:
  Graph g;
  Config cfg;
  GlobalStats stats;
  uint64_t cycles = 0;
  DRAMSimInterface* cb; 
  Cache* cache;
  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;

  int context_to_create = 0;

  /* Resources / limits */
  map<TInstr, int> avail_FUs;
  map<BasicBlock*, int> outstanding_contexts;
  int ports[2]; // ports[0] = loads; ports[1] = stores;
  
  /* Profiled */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  map<int, Context*> curr_owner;
  //map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
  
  /* LSQ */
  LoadStoreQ lsq;

  void toMemHierarchy(DynamicNode* d) {
    cache->addTransaction(d);
  }
 
  void initialize() {
    // Initialize Resources / Limits
    cache = new Cache( cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.block_size, cfg.ideal_cache, NULL, &stats);
    cb = new DRAMSimInterface(cache, cfg.ideal_cache);
    cache->memInterface = cb;
    lsq.size = cfg.lsq_size;
    for(int i=0; i<NUM_INST_TYPES; i++) {
      avail_FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
    }
    if (cfg.cf_mode == 0) 
      context_to_create = 1;
    else if (cfg.cf_mode == 1)  
      context_to_create = cf.size();
    else
      assert(false);
  }

  bool createContext() {
    int cid = context_list.size();
    if (cf.size() == cid) // reached end of <cf> so no more contexts to create
      return false;

    // set "current", "prev", "next" BB ids.
    int bbid = cf.at(cid);
    int next_bbid, prev_bbid;
    if (cf.size() > cid + 1)
      next_bbid = cf.at(cid+1);
    else
      next_bbid = -1;
    if (cid != 0)
      prev_bbid = cf.at(cid-1);
    else
      prev_bbid = -1;
    
    BasicBlock *bb = g.bbs.at(bbid);
    // Check LSQ Availability
    if(!lsq.checkSize(bb->mem_inst_count))
      return false;

    // check the limit of contexts per BB
    if (cfg.max_active_contexts_BB > 0) {
      if(outstanding_contexts.find(bb) == outstanding_contexts.end()) {
        outstanding_contexts.insert(make_pair(bb, cfg.max_active_contexts_BB));
      }
      else if(outstanding_contexts.at(bb) == 0)
        return false;
      outstanding_contexts.at(bb)--;
    }

    Context *c = new Context(cid, this);
    context_list.push_back(c);
    live_context.push_back(c);
    c->initialize(bb, &cfg, next_bbid, prev_bbid);
    return true;
  }

  void process_memory() {
    cb->mem->update();
  }

  bool process_cycle() {
    if(cfg.vInputLevel > 0)
      cout << "[Cycle: " << cycles << "]\n";
    if(cycles % 100000 == 0 && cycles !=0) {
      curr = Clock::now();
      uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr - last).count();
      cout << "Simulation Speed: " << ((double)(stats.num_finished_context - last_processed_contexts)) / tdiff << " contexts per ms \n";
      last_processed_contexts = stats.num_finished_context;
      last = curr;
      stats.num_cycles = cycles;
      stats.print();
    }
    else if(cycles == 0) {
      last = Clock::now();
      last_processed_contexts = 0;
    }
    bool simulate = false;
    ports[0] = cfg.load_ports;
    ports[1] = cfg.store_ports;

    for(auto it = live_context.begin(); it!=live_context.end(); ++it) {
      Context *c = *it;
      c->process();
    }
    for(auto it = live_context.begin(); it!=live_context.end();) {
      Context *c = *it;
      c->complete();
      if(c->live)
        it++;
      else
        it = live_context.erase(it);
    }
    if(live_context.size() > 0)
      simulate = true;
    
    int context_created = 0;
    for (int i=0; i<context_to_create; i++) {
      if ( createContext() ) {
        simulate = true;
        context_created++;
      }
      else
        break;
    }
    context_to_create -= context_created;   // some contexts can be left pending for later cycles
    cache->process_cache();
    cycles++;
    cache->cycles++;
    process_memory();
    return simulate;
  }

  void run() {
    bool simulate = true;
    while (simulate)
      simulate = process_cycle();
    stats.num_cycles = cycles;
    cb->mem->printStats(true);
  }
};


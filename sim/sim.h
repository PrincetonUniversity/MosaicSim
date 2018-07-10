//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "base.h"
#include "include/DRAMSim.h"
#include "functional_cache.h"
#define DEBUGLOG 1

using namespace apollo;
using namespace std;
class Simulator;


class GlobalStats {
public:
  // some global stats
  int num_exec_instr;
  int num_cycles;
  
  GlobalStats() { reset(); }

  void reset() {
    num_exec_instr = 0;
    num_cycles = 0;
  }
  void print() {
    cout << "** Global Stats **\n";
    cout << "num_exec_instr = " << num_exec_instr << endl;
    cout << "num_cycles     = " << num_cycles << endl;
  }
};

class DynamicNode;
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
  std::vector<DynamicNode*> active_list;
  std::set<DynamicNode*> issue_set;
  std::set<DynamicNode*> next_issue_set;
  std::vector<DynamicNode*> next_active_list;
  std::vector<DynamicNode*> nodes_to_complete;
  std::set<DynamicNode*> completed_nodes;

  Context(int id, Simulator* sim) : live(false), id(id), sim(sim) {}
  Context* getNextContext();
  Context* getPrevContext();
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
  int remaining_cycles;
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;

  DynamicNode(Node *n, Context *c, Simulator *sim, Config* cfg, uint64_t addr = 0): n(n), c(c), sim(sim), cfg(cfg), addr(addr) {
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
    remaining_cycles = n->lat;
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
    if(level == 0)
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
      else if(*in < *d)
        return false;
      if(!(d->addr_resolved) && d->n->typeInstr == op_type) {
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


class DRAMSimInterface {
public:
  map< uint64_t, queue<DynamicNode*> > outstanding_accesses_map;
  DRAMSim::MultiChannelMemorySystem *mem;
  DRAMSimInterface() {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);
  }
  
  void read_complete(unsigned id, uint64_t addr, uint64_t mem_clock_cycle) {
    assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
    queue<DynamicNode*> &q = outstanding_accesses_map.at(addr);
    DynamicNode* d = q.front();
    d->handleMemoryReturn();
    q.pop();
    if (q.size() == 0)
      outstanding_accesses_map.erase(addr);
  }
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
    assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
    queue<DynamicNode*> &q = outstanding_accesses_map.at(addr);
    DynamicNode* d = q.front();
    d->handleMemoryReturn();
    q.pop();
    if(q.size() == 0)
      outstanding_accesses_map.erase(addr);
  }
  void addTransaction(DynamicNode* d, uint64_t addr, bool isLoad) {
    mem->addTransaction(!isLoad, addr);
    if(d!= NULL) {
      if(outstanding_accesses_map.find(addr) == outstanding_accesses_map.end())
        outstanding_accesses_map.insert(make_pair(addr, queue<DynamicNode*>()));
      outstanding_accesses_map.at(addr).push(d);
    }
  }
};

class Cache {
public:
  typedef pair<DynamicNode*, uint64_t> CacheOp;
  uint64_t cycles = 0;
  DRAMSimInterface *memInterface;
  int hit_rate=70;
  int latency=2;
  FunctionalSetCache *fc;
  bool ideal=false;
  priority_queue<CacheOp, vector<CacheOp>, less<vector<CacheOp>::value_type> > pq;
  vector<DynamicNode*> ready_to_execute;
  vector<DynamicNode*> next_ready_to_execute;
  vector<pair<DynamicNode*,int>> memop_list, next_memop_list;
  
  struct CacheOpCompare {
    friend bool operator< (const CacheOp &l, const CacheOp &r) {
      if(l.second < r.second) 
        return true;
      else if(l.second == r.second)
        return true;
      else
        return false;
    }
  };

  Cache(int size, int assoc, int block_size, DRAMSimInterface *memInterface, bool ideal): memInterface(memInterface), ideal(ideal) {
    fc = new FunctionalSetCache(size, assoc, block_size);   
    if (ideal)
      latency=1;
  }
  
  /*int isHit() {
    int outcome = rand() % 100;
    if (outcome<hit_rate)        
      return true;
    return false;
  }*/
  void process_cache() {
    cycles++;
    while(pq.size() > 0) {
      if(pq.top().second > cycles)
        break;
      ready_to_execute.push_back(pq.top().first);
      pq.pop();
    }

    for(int i=0; i<ready_to_execute.size(); i++) {
      if(!execute(ready_to_execute.at(i)))
        next_ready_to_execute.push_back(ready_to_execute.at(i));
    }
    ready_to_execute = next_ready_to_execute;
    next_ready_to_execute.clear();
  }
  bool execute(DynamicNode* d) {
    int size_of_cacheline = 64;
    uint64_t dramaddr = d->addr/size_of_cacheline * size_of_cacheline;
    int64_t evictedAddr = -1;
    bool res = true;
    if(!ideal)
      res = fc->access(dramaddr/size_of_cacheline, &evictedAddr);
    if (res) {                  
      d->handleMemoryReturn();
      d->print("Hits in Cache", 2);
    }
    else {//if (memInterface->mem->willAcceptTransaction(dramaddr)) {
      memInterface->addTransaction(d, dramaddr, d->type == LD);
      d->print("Misses in Cache", 2);
    }
    if(evictedAddr != -1) {
      memInterface->addTransaction(NULL, evictedAddr * size_of_cacheline, false);
    }
    return true;
    /* Temporarily disabling rejected transaction */
    //d->print("Transaction Rejected", 2);
    //return false;
  }
  
  void addTransaction(DynamicNode *d) {
    d->print("Added Cache Transaction", 2);
    pq.push(make_pair(d, cycles+latency));   
  }   
};



class Simulator 
{
public:
  Graph g;
  Config cfg;
  GlobalStats stats;
  uint64_t cycles = 0;
  DRAMSimInterface cb= DRAMSimInterface(); 
  Cache* cache;

  vector<Context*> context_list;
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
    //cb.addTransaction(d, d->addr/64 * 64, d->type==LD);
    cache->addTransaction(d);
  }
 
  void initialize() {
    
    // Initialize Resources / Limits
    cache = new Cache( cfg.L1_size, cfg.L1_assoc, cfg.block_size, &cb, cfg.ideal_cache);

    lsq.size = cfg.lsq_size;
    for(int i=0; i<NUM_INST_TYPES; i++) {
      avail_FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
    }

    if (cfg.cf_one_context_at_once) 
      context_to_create = 1;
    else if (cfg.cf_max_contexts_concurrently)  
      context_to_create = cf.size();
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
    
    // check the limit of contexts per BB
    BasicBlock *bb = g.bbs.at(bbid);
    if (cfg.max_active_contexts_BB > 0) {
      if(outstanding_contexts.find(bb) == outstanding_contexts.end()) {
        outstanding_contexts.insert(make_pair(bb, cfg.max_active_contexts_BB));
        outstanding_contexts.at(bb)--;
      }
      else if(outstanding_contexts.at(bb) == 0)
        return false;
    }

    // Check LSQ Availability
    if(!lsq.checkSize(bb->mem_inst_count))
      return false;
    
    Context *c = new Context(cid, this);
    context_list.push_back(c);
    c->initialize(bb, &cfg, next_bbid, prev_bbid);
    return true;
  }

  void process_memory() {
    cb.mem->update();
  }

  bool process_cycle() {
    cout << "[Cycle: " << cycles << "]\n";
    cycles++;
    bool simulate = false;
    //assert(cycles < 10000);
    ports[0] = cfg.load_ports;
    ports[1] = cfg.store_ports;

    for (int i=0; i<context_list.size(); i++) {
      if (context_list.at(i)->live)
        context_list.at(i)->process();
    }

    for (int i=0; i<context_list.size(); i++) 
      if(context_list.at(i)->live) {
        context_list.at(i)->complete();
        if(context_list.at(i)->live)
          simulate = true;
      }

    int context_created = 0;
    for (int i=0; i<context_to_create; i++)   
      if ( createContext() ) {
        simulate = true;
        context_created++;
      }

    context_to_create -= context_created;   // some contexts can be left pending for later cycles
    cache->process_cache();
    process_memory();
    return simulate;
  }

  void run() {
    bool simulate = true;
    while (simulate)
      simulate = process_cycle();
    stats.num_cycles = cycles;
    cb.mem->printStats(false);
  }
};


//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "base.h"
#include "include/DRAMSim.h"
#include "functional_cache.h"
#include <algorithm>
#include <iterator>
#define UNUSED 0
using namespace apollo;
using namespace std;

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;
class Simulator;

class GlobalStats {
  Simulator *sim;
public:
  // some global stats
  int num_exec_instr;
  int num_cycles;
  int num_finished_context;
  int num_L1_hits;
  int num_L1_misses;
  int num_mem_issue_pass = 0;
  int num_mem_issue_try = 0;
  int num_comp_issue_pass = 0;
  int num_comp_issue_try = 0;
  int num_mem_access = 0;
  int num_mem_return = 0;
  int num_mem_evict = 0;
  int num_mem_real = 0;
  int num_misspec = 0;
  int num_mem_hold = 0;
  int memory_events[8];
  GlobalStats(Simulator *sim): sim(sim) { reset(); }

  void reset() {
    num_exec_instr = 0;
    num_cycles = 0;
    num_finished_context = 0;
    num_L1_hits = 0;
    num_L1_misses = 0;
    num_mem_issue_pass = 0;
    num_mem_issue_try = 0;
    num_comp_issue_pass = 0;
    num_comp_issue_try = 0;
    num_mem_access = 0;
    num_mem_return = 0;
    num_mem_evict = 0;
    num_mem_real = 0;
    num_mem_hold = 0;
    num_misspec = 0;
    for(int i=0; i<8; i++)
      memory_events[i] = 0;
  }
  void print();
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
  unsigned int id;

  Simulator* sim;
  Config *cfg;
  BasicBlock *bb;

  int next_bbid;
  int prev_bbid;

  std::map<Node*, DynamicNode*> nodes;
  std::set<DynamicNode*> issue_set;
  std::set<DynamicNode*> waiting_set;
  std::set<DynamicNode*> next_issue_set;
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
  bool addr_resolved = false;
  bool speculated = false;
  int outstanding_accesses = 0;
  /* Depedency */
  int pending_parents;
  int pending_external_parents;
  vector<DynamicNode*> external_dependents;

  DynamicNode(Node *n, Context *c, Simulator *sim, Config *cfg, uint64_t addr = 0): n(n), c(c), sim(sim), cfg(cfg) {
    //if(addr % 4 != 0) 
    //  assert(false);
    //this->addr = addr/4 * 4;
    this->addr = addr;
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
  deque<DynamicNode*> lq;
  deque<DynamicNode*> sq;
  unordered_map<uint64_t, int> lm;
  unordered_map<uint64_t, int> sm;
  int size;
  int invoke[4];
  int traverse[4];
  int count[4];
  set<DynamicNode*> unresolved_st;
  set<DynamicNode*> unresolved_ld;

  LoadStoreQ() {
    invoke[0] = 0;
    invoke[1] = 0;
    invoke[2] = 0;
    invoke[3] = 0;
    traverse[0] = 0;
    traverse[1] = 0;
    traverse[2] = 0;
    traverse[3] = 0;
    count[0] = 0;
    count[1] = 0;
    count[2] = 0;
    count[3] = 0;
    clear();
  }
  void process() {
    for(auto it = sq.begin(); it != sq.end(); ++it) {
      DynamicNode *d = *it;
      if(!d->addr_resolved)
        unresolved_st.insert(d);
    }
    for(auto it = lq.begin(); it != lq.end(); ++it) {
      DynamicNode *d = *it;
      if(!d->addr_resolved)
        unresolved_ld.insert(d);
    } 
  }
  void clear() {
    unresolved_ld.clear();
    unresolved_st.clear();
  }
  void insert(DynamicNode *d) {
    if(d->type == LD) {
      lq.push_back(d);
      if(lm.find(d->addr) == lm.end())
        lm.insert(make_pair(d->addr, 0));
      lm.at(d->addr)++;
    }
    else {
      sq.push_back(d);
      if(sm.find(d->addr) == sm.end())
        sm.insert(make_pair(d->addr, 0));
      sm.at(d->addr)++;
    }
  }
  bool checkSize(int num_ld, int num_st) {
    int ld_need = lq.size() + num_ld - size;
    int st_need = sq.size() + num_st - size; 
    int ld_ct = 0;
    int st_ct = 0;
    if(ld_need <= 0 && st_need <= 0)
      return true;
    if(ld_need > 0) {
      for(auto it = lq.begin(); it!= lq.end(); ++it) {
        if((*it)->completed)
          ld_ct++;
        else
          break;
      }
    }
    if(st_need > 0) {
      for(auto it = sq.begin(); it!= sq.end(); ++it) {
        if((*it)->completed)
          st_ct++;
        else
          break;
      }
    }
    if(ld_ct >= ld_need && st_ct >= st_need) {
      for(int i=0; i<ld_need; i++) {
        lm.at(lq.front()->addr)--;
        if(lm.at(lq.front()->addr) == 0)
          lm.erase(lq.front()->addr);
        lq.pop_front();
      }
      for(int i=0; i<st_need; i++) {
        assert(false);
        sm.at(sq.front()->addr)--;
        if(sm.at(sq.front()->addr) == 0)
          sm.erase(sq.front()->addr);
        sq.pop_front();  
      }
      return true;
    }
    else 
      return false; 
  }
  bool check_unresolved_load(DynamicNode *in) {
    if(unresolved_ld.size() == 0)
      return false;
    if(unresolved_ld.lower_bound(in) != ++unresolved_ld.begin())
      return true;
    else
      return false;
  }
  bool check_unresolved_store(DynamicNode *in) {
    if(unresolved_st.size() == 0)
        return false;
    if(in->type == LD) {
      if(unresolved_st.lower_bound(in) != ++unresolved_st.begin())
        return true;
      else
        return false;
    }
    else if(in->type == ST) {
      if(*(unresolved_st.lower_bound(in)) != in)
        return true;
      else
        return false;
    }
    assert(false);
    return true;
  }
  int check_load_issue(DynamicNode *in, bool speculation_enabled) {
    invoke[0]++;
    bool check = check_unresolved_store(in);
    if(check && !speculation_enabled) {
      count[0]++;
      return -1;
    }
    if(sm.find(in->addr) == sm.end()) {
      if(check)
        return 0;
      else {
        count[3]++;
        return 1;
      }
    }
    // Check Conflict
    for(deque<DynamicNode*>::iterator it = sq.begin(); it!= sq.end(); ++it) {
      DynamicNode *d = *it;
      traverse[0]++;
      if(*in < *d) {
        count[2]++;
        return 1;
      }
      else if(d->addr == in->addr && !d->completed) {
        count[1]++;
        return -1;
      }
    }
    return 1;
  }
  bool check_store_issue(DynamicNode *in) {
    invoke[1]++;
    bool skipStore = false;
    bool skipLoad = false;
    if(check_unresolved_store(in)) {
      return false;
    }
    if(check_unresolved_load(in)) {
      return false;
    }
    if(sm.at(in->addr) == 1)
      skipStore = true;
    if(lm.find(in->addr) == lm.end())
      skipLoad = true;
    if(!skipStore) {
      for(deque<DynamicNode*>::iterator it = sq.begin(); it!= sq.end(); ++it) {
        DynamicNode *d = *it;
        traverse[1]++;
        if((*d == *in) || (*in < *d))
          break;
        else if(d->addr == in->addr && !d->completed) // incomplete instruction with the same address
            return false;
      }
    }
    if(!skipLoad) {
      for(deque<DynamicNode*>::iterator it = lq.begin(); it!= lq.end(); ++it) {
        DynamicNode *d = *it;
        traverse[1]++;
        if(*in < *d)
          break;
        else if(d->addr == in->addr && !d->completed) // incomplete instruction with the same address
          return false;
      }
    }
    return true;
  }
  int check_forwarding (DynamicNode* in) {
    invoke[2]++;
    bool speculative = false;
    auto vit = lower_bound(sq.begin(), sq.end(), in);
    auto rvit = deque<DynamicNode*>::reverse_iterator(vit);
    rvit++;
    for(deque<DynamicNode*>::reverse_iterator it = rvit; it!= sq.rend(); ++it) {
      DynamicNode *d = *it; // traverse from youngest to oldest
      traverse[2]++;
      if(*in < *d) // in is older than d 
        continue;
      if(d->addr == in->addr) {
        if(d->completed && !speculative)
          return 1;
        else if(d->completed && speculative)
          return 0;
        else
          return -1;
      }
      else if(!d->addr_resolved)
        speculative = true;
    }
    return -1;
  }
  std::vector<DynamicNode*> check_speculation (DynamicNode* in) {
    std::vector<DynamicNode*> ret;
    // find younger loads than incoming store
    for(deque<DynamicNode*>::reverse_iterator it = lq.rbegin(); it!= lq.rend(); ++it) {
      DynamicNode *d = *it;
      if(*d < *in) // d is older than in
        break;
      if(d->speculated && d->n->typeInstr == LD && d->addr == in->addr) {
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
  unordered_map< uint64_t, queue<DynamicNode*> > outstanding_read_map;
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
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(DynamicNode* d, uint64_t addr, bool isLoad);
};
class Cache {
public:
  typedef pair<DynamicNode*, uint64_t> Operator;
  FunctionalSetCache *fc;
  vector<DynamicNode*> to_send;
  vector<uint64_t> to_evict;
  priority_queue<Operator, vector<Operator>, less<vector<Operator>::value_type> > pq;
  
  uint64_t cycles = 0;
  int size_of_cacheline = 64;
  
  int latency;
  bool ideal;
  DRAMSimInterface *memInterface;

  GlobalStats *stats;
  
  Cache(int latency, int size, int assoc, bool ideal, GlobalStats *stats): 
            latency(latency), ideal(ideal), stats(stats) {
    fc = new FunctionalSetCache(size, assoc);  
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
        stats->num_mem_access++;
        memInterface->addTransaction(d, dramaddr, true);
        it = to_send.erase(it);
      }
      else
        it++;
    }
    for(auto it = to_evict.begin(); it!= to_evict.end();) {
      uint64_t eAddr = *it;
      if(memInterface->mem->willAcceptTransaction(eAddr)) {
        stats->num_mem_access++;
        stats->num_mem_evict++;
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
void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());
  if(UNUSED)
    cout << id << clock_cycle;
  c->stats->num_mem_return++;
  queue<DynamicNode*> &q = outstanding_read_map.at(addr);
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
    outstanding_read_map.erase(addr);
}
void DRAMSimInterface::write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  if(UNUSED)
    cout << id << addr << clock_cycle;
  c->stats->num_mem_return++;
}
void DRAMSimInterface::addTransaction(DynamicNode* d, uint64_t addr, bool isLoad) {
  if(d!= NULL) {
    assert(isLoad == true);
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<DynamicNode*>()));
      c->stats->num_mem_real++;
      mem->addTransaction(false, addr);
    }
    outstanding_read_map.at(addr).push(d);  
  }
  else {
    assert(isLoad == false);
    mem->addTransaction(true, addr);
  }
}

class Simulator 
{
public:
  Graph g;
  Config cfg;
  GlobalStats stats = GlobalStats(this);
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
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  //map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
  
  /* LSQ */
  LoadStoreQ lsq;

  void toMemHierarchy(DynamicNode* d) {
    cache->addTransaction(d);
  }
 
  void initialize() {
    // Initialize Resources / Limits
    cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.ideal_cache, &stats);
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
    unsigned int cid = context_list.size();
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
    if(!lsq.checkSize(bb->ld_count, bb->st_count))
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
    lsq.process();

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
    lsq.clear();
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

void GlobalStats::print() {
  cout << "** Global Stats **\n";
  cout << "num_exec_instr = " << num_exec_instr << endl;
  cout << "num_cycles     = " << num_cycles << endl;
  cout << "IPC            = " << num_exec_instr / (double)num_cycles << endl;
  cout << "num_finished_context = " << num_finished_context << endl;
  cout << "L1_hits = " << num_L1_hits << endl;
  cout << "L1_misses = " << num_L1_misses << endl;
  cout << "L1_hit_rate = " << num_L1_hits / (double)(num_L1_hits+num_L1_misses) << endl;
  cout << "MemIssue Try : " << num_mem_issue_try << " / " <<"MemIssuePass : " <<  num_mem_issue_pass << " / " << "CompIssueTry : " << num_comp_issue_try << " / " << "CompIssueSuccess : " <<  num_comp_issue_pass << "\n";
  cout << "MemAccess : " << num_mem_access << " / " << "DRAM Access : " << num_mem_real << " / DRAM Return : " << num_mem_return << " / " << "MemEvict : " << num_mem_evict <<  "\n";
  cout << (double)num_mem_real * 64 / (num_cycles/2) << "GB/s \n"; 
  cout << "mem_hold " << num_mem_hold << "\n";
  cout << "lsq: " << sim->lsq.invoke[0] << " / " << sim->lsq.invoke[1] << " / " << sim->lsq.invoke[2] << " / " << sim->lsq.invoke[3] << " \n";
  cout << "lsq: " << sim->lsq.traverse[0] << " / " << sim->lsq.traverse[1] << " / " << sim->lsq.traverse[2] << " / " << sim->lsq.traverse[3] << " \n";
  cout << "lsq3 : " << sim->lsq.count[0] << " / " << sim->lsq.count[1] << " / " << sim->lsq.count[2] << " / " << sim->lsq.count[3] << "\n";
  cout << "misspec: " << sim->stats.num_misspec << "\n";
  for(int i=0; i<8; i++)
    cout << "Memory Event: " << memory_events[i] << "\n";
}
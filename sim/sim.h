//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "base.h"
#include "include/DRAMSim.h"

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

class MemOpInfo {
public:
  uint64_t addr;
  bool addr_resolved = false;
  bool completed = false;
  bool speculated = false;
  int outstanding = 0;  // this keeps track of # in-progress speculative loads. we assume they will come back in the order they were sent.
  MemOpInfo(uint64_t addr): addr(addr) {};
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

  map<Node*, MemOpInfo*> memory_ops;
  map<Node*, vector<std::pair<Node*, Context*>>> external_deps; // Source of external dependency (Node, Destinations for that source (Node, cid))
  std::vector<Node*> active_list;
  std::set<Node*> issue_set;
  std::set<Node*> next_issue_set;
  std::vector<Node *> next_active_list;
  std::vector<Node *> nodes_to_complete;
  std::set<Node*> completed_nodes;

  std::map<Node*, int> remaining_cycles_map;  // tracks remaining cycles for each node
  std::map<Node*, int> pending_parents_map;   // tracks the # of pending parents (intra BB)
  std::map<Node*, int> pending_external_parents_map; // tracks the # of pending parents (across BB)


  Context(int id, Simulator* sim) : live(false), id(id), sim(sim) {}
  //void initialize();
  bool issueMemNode(Node *n);
  bool issueCompNode(Node *n);
  void finishNode(Node *n);
  void tryActivate(Node *n); 
  void handleMemoryReturn(Node *n);
  void process();
  void complete(GlobalStats &stats);
  void initialize(BasicBlock *bb, Config *cfg, int next_bbid, int prev_bbid);
};

typedef std::pair<Node*,Context*> DNode;  // Dynamic Node: a pair of <node,context>

struct DNodeCompare {
  friend bool operator< (const DNode &l, const DNode &r) {
    if(l.second->id < r.second->id) 
      return true;
    else if(l.second->id == r.second->id && l.first->id < r.first->id)
      return true;
    else
      return false;
  }
};

class LoadStoreQ {
public:
  map<DNode, MemOpInfo*> tracker;
  deque<DNode> q;
  int size;
  void insert(DNode d) {
    tracker.insert(make_pair(d, d.second->memory_ops.at(d.first)));
    q.push_back(d);
  }
  bool checkSize(int requested_space) {
    if(q.size() <= size - requested_space) {
      return true;
    }
    else {
      int ct = 0;
      for(deque<DNode>::iterator it = q.begin(); it!= q.end(); ++it) {
        if(tracker.at(*it)->completed)
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
  //negation: every older memop has at least started (knows address)
  bool exists_unresolved_memop (DNode in, TInstr op_type) {
    for(deque<DNode>::iterator it = q.begin(); it!= q.end(); ++it) {
      MemOpInfo *m = tracker.at(*it);
      if(*it == in)
        return false;
      //else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
      else if(*it > in)
        return false;
      if(!(m->addr_resolved) && it->first->typeInstr == op_type) {
        return true;
      }     
    }
    return false;
  }
  //there's at least one addr_resolved, uncompleted older memop with same address
  //negation says for all memops with addr_resolved, uncompleted older memops don't match address
  bool exists_conflicting_memop (DNode in, TInstr op_type) {
    uint64_t addr = tracker.at(in)->addr;
    for(deque<DNode>::iterator it = q.begin(); it!= q.end(); ++it) {
      MemOpInfo *m = tracker.at(*it);
      if(*it == in)
        return false;
      //else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
      else if(*it > in)
        return false;
      if(m->addr_resolved && !(m->completed) &&it->first->typeInstr == op_type && m->addr == addr)
        return true;
    }
    return false;
  }
  int check_forwarding (DNode in) {
    bool found = false;
    bool speculative = false;
    for(deque<DNode>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      MemOpInfo *m = tracker.at(*it);
      if(*it == in) {
        found = true;
        continue;
      }
      if(found) {
        if(m->addr == tracker.at(in)->addr)
          if(m->completed && !speculative)
            return 1;
          else if(m->completed && speculative)
            return 0;
          else
            return -1;
        else if(!m->addr_resolved) {
          speculative = true;
        }
      }
    }
    return -1;
  }
  std::vector<DNode> check_speculation (DNode in) {
    std::vector<DNode> ret;
    uint64_t addr = tracker.at(in)->addr;
    for(deque<DNode>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      MemOpInfo *m = tracker.at(*it);
      if(*it == in)
        break;
      else if(*it < in)
        break;
      if(m->speculated && !(m->completed) && it->first->typeInstr == LD && m->addr == addr) {
        m->speculated = false;
        ret.push_back(*it);
      }
    }
    return ret;
  }
};


class Simulator 
{
public:
  class DRAMSimCallBack {
    public:
      map< uint64_t, queue< DNode > > outstanding_accesses_map;
      Simulator* sim;
      DRAMSimCallBack(Simulator* sim): sim(sim){}
    
      void read_complete(unsigned id, uint64_t addr, uint64_t mem_clock_cycle) {
        assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
        queue<DNode> &q = outstanding_accesses_map.at(addr);
        DNode d = q.front();
        d.second->handleMemoryReturn(d.first);
        //sim->handleMemoryReturn(q.front(), true);
        q.pop();
        if (q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
        assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
        queue<DNode> &q = outstanding_accesses_map.at(addr);
        DNode d = q.front();
        d.second->handleMemoryReturn(d.first);
        //sim->handleMemoryReturn(q.front(), false);
        q.pop();
        if(q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void addTransaction(DNode d, uint64_t addr, bool isLoad) {
        sim->mem->addTransaction(!isLoad, addr);
        if(outstanding_accesses_map.find(addr) == outstanding_accesses_map.end())
          outstanding_accesses_map.insert(make_pair(addr, queue<DNode>()));
        outstanding_accesses_map.at(addr).push(d);
      }
  };

  Graph g;
  Config cfg;
  GlobalStats stats;
  DRAMSimCallBack cb=DRAMSimCallBack(this); 
  DRAMSim::MultiChannelMemorySystem *mem;
  
    vector<Context*> context_list;
  int context_to_create = 0;

  /* Resources */
  map<TInstr, int> FUs;
  int ports[2]; // ports[0] = loads; ports[1] = stores;

  /* Profiled */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  map<Node*, Context*> curr_owner;
  map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
  
  /* LSQ */
  LoadStoreQ lsq;

  Context* getNextContext(int cid) {
    if (context_list.size() > cid+1)
      return context_list.at(cid+1);
    else
      return NULL;
  }

  void initialize() {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);  

    // Initialize Resources
    lsq.size = cfg.lsq_size;
    for(int i=0; i<NUM_INST_TYPES; i++) {
      FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
    }
    // Launch Initial Context
    if (cfg.cf_one_context_at_once) {
      createContext();
    }
    else if (cfg.cf_all_contexts_concurrently)  {
      for ( int i=0; i< cf.size(); i++ )
        createContext();
    }
  }

  void createContext()
  {
    // Create Context
    int cid = context_list.size();
    if(cf.size() == cid)
      return;
    Context *c = new Context(cid, this);
    context_list.push_back(c);
    int bbid = cf.at(cid);
    BasicBlock *bb = g.bbs.at(bbid);
    int next_bbid, prev_bbid;
    if(cf.size() > cid + 1)
      next_bbid = cf.at(cid+1);
    else
      next_bbid = -1;
    if(cid != 0)
      prev_bbid = cf.at(cid-1);
    else
      prev_bbid = -1;
    // Check LSQ Availability
    if(!lsq.checkSize(bb->mem_inst_count))
      assert(false);
    c->initialize(bb, &cfg, next_bbid, prev_bbid);
  }


  void process_memory()
  {
    mem->update();
  }

  bool process_cycle()
  {
    cout << "[Cycle: " << stats.num_cycles << "]\n";
    stats.num_cycles++;
    bool simulate = false;
    assert(stats.num_cycles < 10000);
    ports[0] = cfg.load_ports;
    ports[1] = cfg.store_ports;

    for (int i=0; i<context_list.size(); i++) {
      if (context_list.at(i)->live) {
        context_list.at(i)->process();
      }
    }

    for (int i=0; i<context_list.size(); i++) {
      if(context_list.at(i)->live) {
        context_list.at(i)->complete(stats);
      if(context_list.at(i)->live)
          simulate = true;
      }
    }
    for (int i=0; i<context_to_create; i++) {
      createContext();
      simulate = true;
    }
    context_to_create = 0;
    process_memory();
    return simulate;
  }

  void run()
  {
    bool simulate = true;
    while (simulate)
      simulate = process_cycle();
    mem->printStats(false);
  }
};


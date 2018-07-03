//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "base.h"
#include "include/DRAMSim.h"

using namespace apollo;
using namespace std;
class Simulator;
class Context {
public:
  bool live;
  int id;
  int bbid;
  int processed;
  Simulator* sim;
  Config *cfg;
  std::vector<Node*> active_list;
  std::set<Node*> issue_set;
  std::set<Node*> next_issue_set;
  std::vector<Node *> next_active_list;
  std::vector<Node *> nodes_to_complete;

  std::map<Node*, int> remaining_cycles_map;  // tracks remaining cycles for each node
  std::map<Node*, int> pending_parents_map;   // tracks the # of pending parents (intra BB)
  std::map<Node*, int> pending_external_parents_map; // tracks the # of pending parents (across BB)

  Context(int id, Simulator* sim) : live(true), id(id), bbid(-1), processed(0), sim(sim) {}
  bool issueMemNode(Node *n);
  bool issueCompNode(Node *n);
  void finishNode(Node *n);
  void tryActivate(Node *n); 
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

class MemOp {
public:
  uint64_t addr;
  DNode d;
  bool addr_resolved = false;
  bool completed = false;
  bool speculated = false;
  int outstanding = 0;  // this keeps track of # in-progress speculative loads. we assume they will come back in the order they were sent.
  MemOp(uint64_t addr, DNode d) : addr(addr), d(d){}
};

class LoadStoreQ {
public:
  map<DNode, MemOp*> tracker;
  deque<MemOp*> q;
  bool initialized = false;
  int size;
  void initialize(int s) { 
    size = s;
    initialized = true;
  }
  bool checkSize(int requested_space) {
    if(q.size() <= size - requested_space) {
      return true;
    }
    else {
      int ct = 0;
      for(deque<MemOp*>::iterator it = q.begin(); it!= q.end(); ++it) {
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
  void insert(uint64_t addr, DNode d) {
    assert(initialized);
    MemOp *temp = new MemOp(addr, d);
    q.push_back(temp);
    tracker.insert(make_pair(d, temp));
  }
  //there's at least one unstarted (unknown address) older memop
  //negation: every older memop has at least started (knows address)
  bool exists_unresolved_memop (DNode in, TInstr op_type) {
    for(deque<MemOp*>::iterator it = q.begin(); it!= q.end(); ++it) {
      MemOp *m = (*it);
      if(m->d == in)
        return false;
      //else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
      else if(m->d > in)
        return false;
      if(!(m->addr_resolved) && m->d.first->typeInstr == op_type) {
        return true;
      }     
    }
    return false;
  }
  //there's at least one addr_resolved, uncompleted older memop with same address
  //negation says for all memops with addr_resolved, uncompleted older memops don't match address
  bool exists_conflicting_memop (DNode in, TInstr op_type) {
    uint64_t addr = tracker.at(in)->addr;
    for(deque<MemOp*>::iterator it = q.begin(); it!= q.end(); ++it) {
      MemOp *m = (*it);
      if(m->d == in)
        return false;
      //else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
      else if(m->d > in)
        return false;
      if(m->addr_resolved && !(m->completed) && m->d.first->typeInstr == op_type && m->addr == addr)
        return true;
    }
    return false;
  }
  bool check_forwarding (DNode in) {
    bool found = false;
    for(deque<MemOp*>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      MemOp *m = (*it);
      if(m->d == in) {
        found = true;
        continue;
      }
      if(found) {
        if(m->addr == tracker.at(in)->addr)
          if(m->completed)
            return true;
          else
            return false;
      }
    }
    return false;
  }
  std::vector<DNode> check_speculation (DNode in) {
    std::vector<DNode> ret;
    uint64_t addr = tracker.at(in)->addr;
    for(deque<MemOp*>::reverse_iterator it = q.rbegin(); it!= q.rend(); ++it) {
      MemOp *m = (*it);
      if(m->d == in)
        break;
      else if(m->d < in)
        break;
      if(m->speculated && !(m->completed) && m->d.first->typeInstr == LD && m->addr == addr) {
        m->speculated = false;
        ret.push_back(m->d);
      }
    }
    return ret;
  }
  /*deque<MemOp*>::iterator recent_memop_completed(DNode in, TInstr op_type) {
    deque<MemOp*>::iterator it = q.begin();
    deque<MemOp*>::iterator recent_op = q.end();
    while (it!=q.end() && !((*it)->d.first->id == in.first->id && (*it)->d.second->id == in.second->id) && !((*it)->d.second->id > in.second->id || (((*it)->d.second->id == in.second->id) && ((*it)->d.first->id > in.first->id))) ) { //not end, not equal, not younger
      if ((*it)->d.first->typeInstr==op_type)
        recent_op=it;
      ++it;
    }
    return recent_op;
  }*/
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
        sim->handleMemoryReturn(q.front(), true);
        q.pop();
        if (q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
        assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
        queue<DNode> &q = outstanding_accesses_map.at(addr);
        DNode d = q.front();
        sim->handleMemoryReturn(q.front(), false);
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
  DRAMSimCallBack cb=DRAMSimCallBack(this); 
  DRAMSim::MultiChannelMemorySystem *mem;
  
  int cycle_count = 0;
  vector<Context*> context_list;
  vector<int> context_to_create;
  /* Resources */
  map<TInstr, int> FUs;
  int ports[2]; // ports[0] = loads; ports[1] = stores;

  vector<int> cf; // List of basic blocks in "sequential" program order 
  map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  /* Handling External Dependencies */
  map<Node*, pair<int, bool> > curr_owner; // Source of external dependency (Node), Context ID for the node (cid), Finished
  map<DNode, vector<DNode> > deps; // Source of external dependency (Node,cid), Destinations for that source (Node, cid)
  /* Handling Phi Dependencies */
  map<int, set<Node*> > handled_phi_deps; // Context ID, Processed Phi node
  
  LoadStoreQ lsq;
  Context* getNextContext(int cid) {
    if (cf.size() > cid+1 && context_list.size() > cid+1)
      return context_list.at(cid+1);
    else
      return NULL;
  }
  int getNextBasicBlock(int cid) {
    if(cf.size() > cid+1) 
      return cf.at(cid+1);
    else
      return -1;
  }
  void initialize();
  void createContext(int bbid);
  void handleMemoryReturn(DNode d, bool isLoad);
  void complete_context(Context *c);
  void process_context(Context *C);
  bool process_cycle();
  void process_memory();
  void run();
};

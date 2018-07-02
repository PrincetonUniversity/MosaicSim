//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim-apollo.hpp"
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
  std::vector<Node*> active_list;
  std::set<Node*> issue_set;
  std::set<Node*> next_issue_set;
  std::vector<Node *> next_active_list;
  std::vector<Node *> nodes_to_complete;

  std::map<Node*, int> remaining_cycles_map;  // tracks remaining cycles for each node
  std::map<Node*, int> pending_parents_map;   // tracks the # of pending parents (intra BB)
  std::map<Node*, int> pending_external_parents_map; // tracks the # of pending parents (across BB)

  Context(int id, Simulator* sim) : live(true), id(id), bbid(-1), processed(0), sim(sim) {}
  void tryActivate(Node* parent, Node *n); 
};


typedef std::pair<Node*,Context*> DNode;  // Dynamic Node: a pair of <node,context>


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
      else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
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
      else if(m->d.second->id > in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id > in.first->id)))
        return false;
      if(m->addr_resolved && !(m->completed) && m->d.first->typeInstr == op_type && m->addr == addr)
        return true;
    }
    return false;
  }

  deque<MemOp*>::iterator recent_memop_completed(DNode in, TInstr op_type) {
    deque<MemOp*>::iterator it = q.begin();
    deque<MemOp*>::iterator recent_op = q.end();
    while (it!=q.end() && !((*it)->d.first->id == in.first->id && (*it)->d.second->id == in.second->id) && !((*it)->d.second->id > in.second->id || (((*it)->d.second->id == in.second->id) && ((*it)->d.first->id > in.first->id))) ) { //not end, not equal, not younger
      if ((*it)->d.first->typeInstr==op_type)
        recent_op=it;
      ++it;
    }
    return recent_op;
  }
  
  std::vector<DNode> check_speculation (DNode in) {
    std::vector<DNode> ret;
    uint64_t addr = tracker.at(in)->addr;
    for(deque<MemOp*>::iterator it = q.end(); it!= q.begin(); --it) {
      MemOp *m = (*it);
      if(m->d == in)
        break;
      else if(m->d.second->id < in.second->id || ((m->d.second->id == in.second->id) && (m->d.first->id < in.first->id)))
        break;
      if(m->speculated && !(m->completed) && m->d.first->typeInstr == LD && m->addr == addr) {
        m->speculated = false;
        ret.push_back(m->d);
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
        cout << "Node [" << d.first->name << " @ context " << d.second->id << "]: Memory Transaction Returns \n";
        sim->handleMemoryReturn(q.front(), true);
        q.pop();
        if (q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
        assert(outstanding_accesses_map.find(addr) != outstanding_accesses_map.end());
        queue<DNode> &q = outstanding_accesses_map.at(addr);
        DNode d = q.front();
        cout << "Node [" << d.first->name << " @ context " << d.second->id << "]: Memory Transaction Returns \n";
        sim->handleMemoryReturn(q.front(), false);
        q.pop();
        if(q.size() == 0)
          outstanding_accesses_map.erase(addr);
      }
      void addTransaction(DNode d, uint64_t addr, bool isLoad) {
        cout << "Node [" << d.first->name << " @ context " << d.second->id << "]: Inserts Memory Transaction for Address "<< addr << "\n";
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
  bool mem_spec_mode=false;
  bool store_load_fwd=true;

  void initialize() {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);  
    lsq.initialize(cfg.lsq_size);
    for(int i=0; i<NUM_INST_TYPES; i++) {
      FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
    }
    if (cfg.CF_one_context_at_once) {
      createContext( cf.at(0) );  // create just the very firt context from <cf>
    }
    else if (cfg.CF_all_contexts_concurrently)  { // create ALL contexts to max parallelism
      for ( int i=0; i< cf.size(); i++ )
        createContext( cf.at(i) );
    }
  }

  void createContext(int bbid)
  {
    assert(bbid < g.bbs.size());
    /* Create Context */
    int cid = context_list.size();
    Context *c = new Context(cid, this);
    context_list.push_back(c);
    BasicBlock *bb = g.bbs.at(bbid);
    // Initialize
    c->bbid = bb->id;
    int mem_count = 0;
    for ( int i=0; i<bb->inst.size(); i++ ) {
      Node *n = bb->inst.at(i);
      if (n->typeInstr == ST || n->typeInstr == LD)
        mem_count++;
    }
    if(!lsq.checkSize(mem_count))
      assert(false); // Context creation failed due to limited LSQ size
    c->active_list.push_back(bb->entry);
    c->remaining_cycles_map.insert(std::make_pair(bb->entry, 0));
    
    // traverse all the BB's instructions and initialize the pending_parents map
    for ( int i=0; i<bb->inst.size(); i++ ) {
      Node *n = bb->inst.at(i);
      if (n->typeInstr == ST || n->typeInstr == LD) {
        // add entries to LSQ
        if(memory.find(n->id) == memory.end()) {
          cout << "Can't find Corresponding Memory Input (" << n->id << ")\n";
          assert(false);
        }
        uint64_t addr = memory.at(n->id).front();
        lsq.insert(addr, make_pair(n, c));
        memory.at(n->id).pop();
      }
          
      if(n->typeInstr == PHI)
        c->pending_parents_map.insert(std::make_pair(n, n->parents.size()+1));
      else
        c->pending_parents_map.insert(std::make_pair(n, n->parents.size()));
      c->pending_external_parents_map.insert(std::make_pair(n, n->external_parents.size()));
    }

    /* Handle Phi */
    if (handled_phi_deps.find(c->id) != handled_phi_deps.end()) {
      std::set<Node*>::iterator it;
      for(it = handled_phi_deps.at(c->id).begin(); it != handled_phi_deps.at(c->id).end(); ++it) {
        c->pending_parents_map.at(*it)--;
      }
      handled_phi_deps.erase(c->id);
    }

    /* Handle External Edges */
    for (int i=0; i<bb->inst.size(); i++) {
      Node *n = bb->inst.at(i);
      if (n->external_dependents.size() > 0) {
        if(curr_owner.find(n) == curr_owner.end())
          curr_owner.insert(make_pair(n, make_pair(c->id, false)));
        else {
          curr_owner.at(n).first = c->id;
          curr_owner.at(n).second = false;
        }
      }
      if (n->external_parents.size() > 0) {
        std::set<Node*>::iterator it;
        for (it = n->external_parents.begin(); it != n->external_parents.end(); ++it) {
          Node *s = *it;
          int src_cid = curr_owner.at(s).first;
          bool done = curr_owner.at(s).second;
          DNode src = make_pair(s, context_list.at(src_cid));
          if(done)
            c->pending_external_parents_map.at(n)--;
          else {
            if(deps.find(src) == deps.end()) {
              deps.insert(make_pair(src, vector<DNode>()));
            }
            deps.at(src).push_back(make_pair(n, c));
          }
        }
      }
    }
    //c->initialize(bb, curr_owner, deps, handled_phi_deps, lsq, memory);
    cout << "Context [" << cid << "]: Created (BB=" << bbid << ")\n";     
  }

  void handleMemoryReturn(DNode d, bool isLoad) {
    if(isLoad) {
      if(mem_spec_mode) {
        if(lsq.tracker.at(d)->outstanding > 0)
          lsq.tracker.at(d)->outstanding--;
        if(lsq.tracker.at(d)->outstanding != 0)
          return;
      }
      d.second->remaining_cycles_map.at(d.first) = 0; // mark "Load" as finished
    }
  }
  bool issueCompNode(Node *n, Context *c) {
    bool canExecute = true;
    // check resource (FU) availability
    if (FUs.at(n->typeInstr) != -1) {
      if (FUs.at(n->typeInstr) == 0)
        canExecute = false;
    }
    if(canExecute) {
      if (FUs.at(n->typeInstr) != -1)
        FUs.at(n->typeInstr)--;
      return true;
    }
    else
      return false;
  }
  bool issueMemNode(Node *n, Context *c) {
    // Memory Dependency
    DNode d = make_pair(n,c);
    bool stallCondition = false;
    lsq.tracker.at(d)->addr_resolved = true;
    
    bool canExecute = true;
    bool speculate = false;
    bool forward = false;
    deque<MemOp*>::iterator prev_store;
    if (n->typeInstr == LD) {
      prev_store = lsq.recent_memop_completed(d,ST);
      forward = store_load_fwd && prev_store!=lsq.q.end() && (*prev_store)->completed;  
    } 
    // Memory Ports
    if (n->typeInstr == LD && ports[0] == 0) //no need for mem ports if you can fwd from store buffer
      canExecute = false;
    if (n->typeInstr == ST && ports[1] == 0)
      canExecute = false;

    bool exists_unresolved_ST = lsq.exists_unresolved_memop(d, ST);
    bool exists_conflicting_ST = lsq.exists_conflicting_memop(d, ST);
    if (n->typeInstr == ST) {
      bool exists_unresolved_LD = lsq.exists_unresolved_memop(d, LD);
      bool exists_conflicting_LD = lsq.exists_conflicting_memop(d, LD);
      stallCondition = exists_unresolved_ST || exists_conflicting_ST || exists_unresolved_LD || exists_conflicting_LD;
    }
    else if (n->typeInstr == LD) {
      if(mem_spec_mode) {
        stallCondition = exists_conflicting_ST;
        speculate = exists_unresolved_ST && !exists_conflicting_ST && !forward; //if you can fwd, you're not speculating
      }
      else
        stallCondition = exists_unresolved_ST || exists_conflicting_ST;
    }
    
    canExecute &= !stallCondition;
    uint64_t addr = lsq.tracker.at(d)->addr;
    uint64_t dramaddr = (addr/64) * 64;
    canExecute &= mem->willAcceptTransaction(dramaddr) || forward; //if you can forward, you can always execute

    // Issue Successful
    if(canExecute) {
      DNode d = make_pair(n,c);
      lsq.tracker.at(d)->speculated = speculate;  
      if (n->typeInstr == LD) {
        ports[0]--;
        if(forward) {
          cout << "For Address " << addr << " Node [" << (*prev_store)->d.first->name << " @ context " << (*prev_store)->d.second->id << "]: Forwards Data to Node [" << d.first->name << " @ context " << d.second->id << "] \n";
          handleMemoryReturn(d,true); //we're done with this operation
        }
        else
          cb.addTransaction(d, dramaddr, true);
        if (speculate)
          lsq.tracker.at(d)->outstanding++; //increase number of outstanding loads by that node
      }
      else if (n->typeInstr == ST) {
        ports[1]--;
        cb.addTransaction(d, dramaddr, false);
      }
      return true;
    }
    else
      return false;
  }
  void finishNode(Node *n, Context *c) {
    DNode d = make_pair(n,c);
    cout << "Node [" << n->name << " @ context " << c->id << "]: Finished Execution \n";

    c->remaining_cycles_map.erase(n);
    if (n->typeInstr != ENTRY)
      c->processed++;

    // Handle Resource
    if ( FUs.at(n->typeInstr) != -1 ) { // if FU is "limited" -> realease the FU
      FUs.at(n->typeInstr)++; 
      cout << "Node [" << n->name << "]: released FU, new free FU: " << FUs.at(n->typeInstr) << endl;
    }

    // Speculation
    if (mem_spec_mode && n->typeInstr == ST) {
      auto misspeculated = lsq.check_speculation(d);
      for(int i=0; i<misspeculated.size(); i++) {
        // Handle Misspeculation
        Context *cc = misspeculated.at(i).second;
        cc->tryActivate(n,misspeculated.at(i).first);
      }
    }

    if (n->typeInstr == LD || n->typeInstr == ST) {         
      assert(lsq.tracker.at(d)->outstanding==0);
      lsq.tracker.at(d)->speculated = false;
      lsq.tracker.at(d)->completed = true;
    }

    // Since node <n> ended, update dependents: decrease each dependent's parent count & try to launch each dependent
    set<Node*>::iterator it;
    for (it = n->dependents.begin(); it != n->dependents.end(); ++it) {
      Node *d = *it;
      c->pending_parents_map.at(d)--;
      c->tryActivate(n,d);    
    }

    // The same for external dependents: decrease parent's count & try to launch them
    if (n->external_dependents.size() > 0) {
      DNode src = make_pair(n, c);
      if (deps.find(src) != deps.end()) {
        vector<DNode> users = deps.at(src);
        for (int i=0; i<users.size(); i++) {
          Node *d = users.at(i).first;
          Context *cc = users.at(i).second;
          cc->pending_external_parents_map.at(d)--;
          cc->tryActivate(n,d);
        }
        deps.erase(src);
      }
      curr_owner.at(n).second = true;
    }

    // Same for Phi dependents
    for (it = n->phi_dependents.begin(); it != n->phi_dependents.end(); ++it) {
      int next_cid = c->id+1;
      Node *d = *it;
      if ((cf.size() > next_cid) && (cf.at(next_cid) == d->bbid)) {
        if (context_list.size() > next_cid) {
          Context *cc = context_list.at(next_cid);
          cc->pending_parents_map.at(d)--;
          cc->tryActivate(n,d);
        }
        else {
          if (handled_phi_deps.find(next_cid) == handled_phi_deps.end()) {
            handled_phi_deps.insert(make_pair(next_cid, set<Node*>()));
          }
          handled_phi_deps.at(next_cid).insert(d);
        }
      }
    }

    // If node <n> is a TERMINATOR create new context with next bbid in <cf> (a bbid list)
    if (cfg.CF_one_context_at_once && n->typeInstr == TERMINATOR) {
      if ( c->id < cf.size()-1 ) {  // if there are more pending contexts in the <cf> vector
        context_to_create.push_back(cf.at(c->id + 1));
      }
    }
  }

  void complete_context(Context *c)
  {
    for(int i=0; i<c->nodes_to_complete.size(); i++) {
      Node *n = c->nodes_to_complete.at(i);
      finishNode(n,c);
      // Check if the current context is done
      if ( c->processed == g.bbs.at(c->bbid)->inst_count ) {
        cout << "Context [" << c->id << "]: Finished Execution (Executed " << c->processed << " instructions) \n";
        c->live = false;
      }
    }
    c->nodes_to_complete.clear();
  }
  
  void process_context(Context *c)
  {
    // traverse ALL the active instructions in the context
    for (int i=0; i < c->active_list.size(); i++) {
      Node *n = c->active_list.at(i);
      if (c->issue_set.find(n) != c->issue_set.end()) {
        bool res = false;
        if(n->typeInstr == LD || n->typeInstr == ST)
          res = issueMemNode(n,c);
        else
          res = issueCompNode(n,c);
        if(!res) {
          //cout << "Node [" << n->name << " @ context " << c->id << "]: Issue failed \n";
          c->next_active_list.push_back(n);
          c->next_issue_set.insert(n);
          continue;
        }
        else
          cout << "Node [" << n->name << " @ context " << c->id << "]: Issue successful \n";
      }

      // decrease the remaining # of cycles for current node <n>
      int remaining_cycles = c->remaining_cycles_map.at(n);
      if (remaining_cycles > 0)
        remaining_cycles--;
      
      DNode d = make_pair(n,c);
            
      if ( remaining_cycles > 0 || remaining_cycles == -1 ) {  // Node <n> will continue execution in next cycle
        c->next_active_list.push_back(n);
        continue;
      }      
      else if (remaining_cycles == 0) { // Execution finished for node <n>
        // Check if speculatied load can be confirmed to be correct
        if (mem_spec_mode && n->typeInstr == LD && lsq.tracker.at(d)->speculated) {
          assert(lsq.tracker.at(d)->outstanding == 0);
          bool exists_unresolved_ST = lsq.exists_unresolved_memop(d, ST);
          if (exists_unresolved_ST) { 
            c->next_active_list.push_back(n);
            continue;
          }
        }
        c->nodes_to_complete.push_back(n);
      }
      else
        assert(false);
    }

    // Continue with following active instructions
    c->issue_set = c->next_issue_set;
    c->active_list = c->next_active_list;
    c->next_issue_set.clear();
    c->next_active_list.clear();
  }
  void process_memory()
  {
    mem->update();
  }

  bool process_cycle()
  {
    cout << "[Cycle: " << cycle_count << "]\n";
    cycle_count++;
    bool simulate = false;
    assert(cycle_count < 10000);
    ports[0] = cfg.load_ports;
    ports[1] = cfg.store_ports;
    // process all live contexts
    for (int i=0; i<context_list.size(); i++) {
      if (context_list.at(i)->live) {
        process_context(context_list.at(i));
      }
    }
    for (int i=0; i<context_list.size(); i++) {
      if (context_list.at(i)->live) {
        complete_context(context_list.at(i));
        if(context_list.at(i)->live)
          simulate = true;
      }
    }
    for (int i=0; i<context_to_create.size(); i++) {
      createContext(context_to_create.at(i));
      simulate = true;
    }
    context_to_create.clear();
    process_memory();
    return simulate;
  }

  void run()
  {
    bool simulate = true;
    while (simulate) {
      simulate = process_cycle();
    }
    mem->printStats(false);
  }
};

void Context::tryActivate(Node* parent, Node *n) {
    if (n->typeInstr==ST && parent==n->addr_operand) 
      sim->lsq.tracker.at(make_pair(n,this))->addr_resolved=true;      
    
    if(pending_parents_map.at(n) > 0 || pending_external_parents_map.at(n) > 0) {
      //std::cout << "Node [" << n->name << " @ context " << id << "]: Failed to Execute - " << pending_parents_map.at(n) << " / " << pending_external_parents_map.at(n) << "\n";
      return;
    }
    active_list.push_back(n);
    if(issue_set.find(n) != issue_set.end())
      assert(false); // error : activate to the same node twice
    issue_set.insert(n);
    std::cout << "Node [" << n->name << " @ context " << id << "]: Added to active list\n";
    remaining_cycles_map.insert(std::make_pair(n, n->lat));
}

int main(int argc, char const *argv[])
{
  Simulator sim;
  sim.mem_spec_mode= true;
  Reader r;
  r.readCfg("sim/config/config.txt", sim.cfg);
  // Workload 
  if(argc != 2)
    assert(false);
  cout << "Path: " << argv[1] << "\n";
  string s(argv[1]);

  string gname = s + "/output/graphOutput.txt";
  string mname = s + "/output/mem.txt";
  string cname = s + "/output/ctrl.txt";
 
  r.readGraph(gname, sim.g, sim.cfg);
  r.readProfMemory(mname , sim.memory);
  r.readProfCF(cname, sim.cf);
  
  /*
  r.readGraph("../workloads/toy/toygraph.txt", sim.g, sim.cfg);
  r.readProfMemory("../workloads/toy/toymem.txt", sim.memory);
  r.readProfCF("../workloads/toy/toycf.txt", sim.cf); 
  */
  
  sim.initialize();
  sim.run();
  return 0;
} 

//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim.h"

using namespace apollo;
using namespace std;

void Simulator::initialize() {
  DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::read_complete);
  DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimCallBack, void, unsigned, uint64_t, uint64_t>(&cb, &DRAMSimCallBack::write_complete);
  mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
  mem->RegisterCallbacks(read_cb, write_cb, NULL);
  mem->setCPUClockSpeed(2000000000);  
  lsq.initialize(cfg.lsq_size);
  // Initialize Resources
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
void Simulator::createContext()
{
  // Create Context
  int cid = context_list.size();
  if(cf.size() == cid)
    return;
  Context *c = new Context(cid, this);
  context_list.push_back(c);
  int bbid = cf.at(cid);
  BasicBlock *bb = g.bbs.at(bbid);

  c->bb = bb;
  c->cfg = &cfg;
  
  // Check LSQ Availability
  int mem_count = 0;
  for ( int i=0; i<bb->inst.size(); i++ ) {
    Node *n = bb->inst.at(i);
    if (n->typeInstr == ST || n->typeInstr == LD)
      mem_count++;
  }
  if(!lsq.checkSize(mem_count))
    assert(false);

  c->active_list.push_back(bb->entry);
  c->remaining_cycles_map.insert(std::make_pair(bb->entry, 0));
  
  // Initialize Context Structures
  for ( int i=0; i<bb->inst.size(); i++ ) {
    Node *n = bb->inst.at(i);
    if (n->typeInstr == ST || n->typeInstr == LD) {
      // Initialize entries in LSQ
      if(memory.find(n->id) == memory.end()) {
        cout << "Can't find Corresponding Memory Input (" << n->id << ")\n";
        assert(false);
      }
      uint64_t addr = memory.at(n->id).front();
      lsq.insert(addr, make_pair(n,c));
      memory.at(n->id).pop();
    }
    if(n->typeInstr == PHI)
      c->pending_parents_map.insert(std::make_pair(n, n->parents.size()+1));
    else
      c->pending_parents_map.insert(std::make_pair(n, n->parents.size()));
    c->pending_external_parents_map.insert(std::make_pair(n, n->external_parents.size()));
  }

  // Initialize Phi Dependency
  if (handled_phi_deps.find(c->id) != handled_phi_deps.end()) {
    std::set<Node*>::iterator it;
    for(it = handled_phi_deps.at(c->id).begin(); it != handled_phi_deps.at(c->id).end(); ++it) {
      c->pending_parents_map.at(*it)--;
    }
    handled_phi_deps.erase(c->id);
  }

  // Initialize External Edges
  for (int i=0; i<bb->inst.size(); i++) {
    Node *n = bb->inst.at(i);
    if (n->external_dependents.size() > 0) {
      if(curr_owner.find(n) == curr_owner.end())
        curr_owner.insert(make_pair(n, c));
      else {
        curr_owner.at(n) = c;
      }
      c->external_deps.insert(make_pair(n, vector<DNode>()));
    }
    if (n->external_parents.size() > 0) {
      std::set<Node*>::iterator it;
      for (it = n->external_parents.begin(); it != n->external_parents.end(); ++it) {
        Node *s = *it;
        Context *cc = curr_owner.at(s);
        if(cc->completed_nodes.find(s) != cc->completed_nodes.end())
          c->pending_external_parents_map.at(n)--;
        else {
          cc->external_deps.at(s).push_back(make_pair(n,c));
        }
      }
    }
  }
  cout << "Context [" << cid << "]: Created (BB=" << bbid << ")\n";     
}

void Simulator::handleMemoryReturn(DNode d, bool isLoad) {
  cout << "Node [" << d.first->name << " @ context " << d.second->id << "]: Memory Transaction Returns \n";
  if(isLoad) {
    if(cfg.mem_speculate) {
      if(lsq.tracker.at(d)->outstanding > 0)
        lsq.tracker.at(d)->outstanding--;
      if(lsq.tracker.at(d)->outstanding != 0)
        return;
    }
    d.second->remaining_cycles_map.at(d.first) = 0; // mark "Load" as finished
  }
}

void Simulator::complete_context(Context *c)
{
  for(int i=0; i<c->nodes_to_complete.size(); i++) {
    Node *n = c->nodes_to_complete.at(i);
    c->finishNode(n);
    if (c->completed_nodes.size() == c->bb->inst_count) {
      cout << "Context [" << c->id << "]: Finished Execution (Executed " << c->completed_nodes.size() << " instructions) \n";
      c->live = false;
    }
  }
  c->nodes_to_complete.clear();
}

void Simulator::process_context(Context *c)
{
  for (int i=0; i < c->active_list.size(); i++) {
    Node *n = c->active_list.at(i);
    if (c->issue_set.find(n) != c->issue_set.end()) {
      bool res = false;
      if(n->typeInstr == LD || n->typeInstr == ST)
        res = c->issueMemNode(n);
      else
        res = c->issueCompNode(n);
      if(!res) {
        //cout << "Node [" << n->name << " @ context " << c->id << "]: Issue failed \n";
        c->next_active_list.push_back(n);
        c->next_issue_set.insert(n);
        continue;
      }
      else
        cout << "Node [" << n->name << " @ context " << c->id << "]: Issue successful \n";
    }

    // Update Remaining Cycles
    int remaining_cycles = c->remaining_cycles_map.at(n);
    if (remaining_cycles > 0)
      remaining_cycles--;
    
    DNode d = make_pair(n,c);
    // Continue Execution
    if ( remaining_cycles > 0 || remaining_cycles == -1 ) {
      c->next_active_list.push_back(n);
      continue;
    }      
    else if (remaining_cycles == 0) { 
      // Execution Finished
      // Check the speculation result for speculative loads
      if (cfg.mem_speculate && n->typeInstr == LD && lsq.tracker.at(d)->speculated) {
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
  c->issue_set = c->next_issue_set;
  c->active_list = c->next_active_list;
  c->next_issue_set.clear();
  c->next_active_list.clear();
}
void Simulator::process_memory()
{
  mem->update();
}

bool Simulator::process_cycle()
{
  cout << "[Cycle: " << cycle_count << "]\n";
  cycle_count++;
  bool simulate = false;
  assert(cycle_count < 10000);
  ports[0] = cfg.load_ports;
  ports[1] = cfg.store_ports;

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
  for (int i=0; i<context_to_create; i++) {
    createContext();
    simulate = true;
  }
  context_to_create = 0;
  process_memory();
  return simulate;
}

void Simulator::run()
{
  bool simulate = true;
  while (simulate)
    simulate = process_cycle();
  mem->printStats(false);
}
/*
bool Context::issueNode(Node *n, bool isMem) {

}*/
bool Context::issueCompNode(Node *n) {
  bool canExecute = true;
  // check resource (FU) availability
  if (sim->FUs.at(n->typeInstr) != -1) {
    if (sim->FUs.at(n->typeInstr) == 0)
      canExecute = false;
  }
  if(canExecute) {
    if (sim->FUs.at(n->typeInstr) != -1)
      sim->FUs.at(n->typeInstr)--;
    return true;
  }
  else
    return false;
}
bool Context::issueMemNode(Node *n) {
  // Memory Dependency
  DNode d = make_pair(n,this);
  sim->lsq.tracker.at(d)->addr_resolved = true;
  bool stallCondition = false;  
  bool canExecute = true;
  bool speculate = false;
  int forwardRes = -1;

  uint64_t addr = sim->lsq.tracker.at(d)->addr;
  uint64_t dramaddr = (addr/64) * 64;    
  
  bool exists_unresolved_ST = sim->lsq.exists_unresolved_memop(d, ST);
  bool exists_conflicting_ST = sim->lsq.exists_conflicting_memop(d, ST);
  if (n->typeInstr == ST) {
    bool exists_unresolved_LD = sim->lsq.exists_unresolved_memop(d, LD);
    bool exists_conflicting_LD = sim->lsq.exists_conflicting_memop(d, LD);
    stallCondition = exists_unresolved_ST || exists_conflicting_ST || exists_unresolved_LD || exists_conflicting_LD;
  }
  else if (n->typeInstr == LD) {
    if(cfg->mem_forward)
      forwardRes = sim->lsq.check_forwarding(d);
    if(forwardRes == 1) {
      canExecute = true;
    }
    else if(forwardRes == 0) {
      if(cfg->mem_speculate) {
        canExecute = true;
        speculate = true;
      }
      else {
        canExecute = false;
      }
    }
    else if(forwardRes == -1) {
      if(cfg->mem_speculate) {
        stallCondition = exists_conflicting_ST;
        speculate = exists_unresolved_ST && !exists_conflicting_ST; //if you can fwd, you're not speculating
      }
      else
        stallCondition = exists_unresolved_ST || exists_conflicting_ST;
    }
  }
  if(forwardRes == -1) {
    if (n->typeInstr == LD && sim->ports[0] == 0) //no need for mem ports if you can fwd from store buffer
      canExecute = false;
    if (n->typeInstr == ST && sim->ports[1] == 0)
      canExecute = false;
    canExecute &= !stallCondition;
    canExecute &= sim->mem->willAcceptTransaction(dramaddr); 
  }
  // Issue Successful
  if(canExecute) {
    DNode d = make_pair(n,this);
    sim->lsq.tracker.at(d)->speculated = speculate;  
    if (n->typeInstr == LD) {
      if(forwardRes == 1) { 
        cout << "Node [" << n->name << " @ context " << id << "] retrieves forwarded Data \n";
        d.second->remaining_cycles_map.at(d.first) = 0;
        //cout << "For Address " << addr << " Node [" << (*prev_store)->d.first->name << " @ context " << (*prev_store)->d.second->id << "]: Forwards Data to Node [" << d.first->name << " @ context " << d.second->id << "] \n";
      }
      else if(forwardRes == 0) { 
        cout << "Node [" << n->name << " @ context " << id << "] speculuatively retrieves forwarded data \n";
      }
      else if(forwardRes == -1) { 
        sim->ports[0]--;
        sim->cb.addTransaction(d, dramaddr, true);
        if (speculate)
          sim->lsq.tracker.at(d)->outstanding++; //increase number of outstanding loads by that node
      }
    }
    else if (n->typeInstr == ST) {
      sim->ports[1]--;
      sim->cb.addTransaction(d, dramaddr, false);
    }
    cout << "Node [" << d.first->name << " @ context " << d.second->id << "]: Inserts Memory Transaction for Address "<< dramaddr << "\n";
    return true;
  }
  else
    return false;
}
void Context::finishNode(Node *n) {
  DNode d = make_pair(n,this);
  cout << "Node [" << n->name << " @ context " << id << "]: Finished Execution \n";

  remaining_cycles_map.erase(n);
  if (n->typeInstr != ENTRY)
    completed_nodes.insert(n);

  // Handle Resource
  if ( sim->FUs.at(n->typeInstr) != -1 ) { // if FU is "limited" -> release the FU
    sim->FUs.at(n->typeInstr)++; 
    cout << "Node [" << n->name << "]: released FU, new free FU: " << sim->FUs.at(n->typeInstr) << endl;
  }

  // Speculation
  if (cfg->mem_speculate && n->typeInstr == ST) {
    auto misspeculated = sim->lsq.check_speculation(d);
    for(int i=0; i<misspeculated.size(); i++) {
      // Handle Misspeculation
      Context *cc = misspeculated.at(i).second;
      cc->tryActivate(misspeculated.at(i).first);
    }
  }

  if (n->typeInstr == LD || n->typeInstr == ST) {         
    assert(sim->lsq.tracker.at(d)->outstanding==0);
    sim->lsq.tracker.at(d)->speculated = false;
    sim->lsq.tracker.at(d)->completed = true;
  }

  // Since node <n> ended, update dependents: decrease each dependent's parent count & try to launch each dependent
  set<Node*>::iterator it;
  for (it = n->dependents.begin(); it != n->dependents.end(); ++it) {
    Node *d = *it;
    pending_parents_map.at(d)--;
    if(n->store_addr_dependents.find(d) != n->store_addr_dependents.end()) {
      sim->lsq.tracker.at(make_pair(d,this))->addr_resolved = true;
    }
    tryActivate(d);    
  }

  // The same for external dependents: decrease parent's count & try to launch them
  if (n->external_dependents.size() > 0) {
    if (external_deps.at(n).size() > 0) {
      vector<DNode> users = external_deps.at(n);
      for (int i=0; i<users.size(); i++) {
        Node *d = users.at(i).first;
        Context *cc = users.at(i).second;
        if(n->store_addr_dependents.find(d) != n->store_addr_dependents.end()) {
          sim->lsq.tracker.at(make_pair(d,cc))->addr_resolved = true;
        }
        pending_external_parents_map.at(d)--;
        cc->tryActivate(d);
      }
      external_deps.erase(n);
    }
  }

  // Same for Phi dependents
  for (it = n->phi_dependents.begin(); it != n->phi_dependents.end(); ++it) {
    Node *d = *it;
    if(sim->getNextBasicBlock(id) == d->bbid) {
      if(Context *cc = sim->getNextContext(id)) {
        pending_parents_map.at(d)--;
        cc->tryActivate(d);
      }
      else {
        if (sim->handled_phi_deps.find(id+1) == sim->handled_phi_deps.end()) {
          sim->handled_phi_deps.insert(make_pair(id+1, set<Node*>()));
        }
        sim->handled_phi_deps.at(id+1).insert(d);
      }
    }
  }

  // If node <n> is a TERMINATOR create new context with next bbid in <cf> (a bbid list)
  if (cfg->cf_one_context_at_once && n->typeInstr == TERMINATOR) {
    sim->context_to_create++;
  }
}

void Context::tryActivate(Node *n) {
    //if (n->typeInstr==ST && parent==n->addr_operand) 
    //  sim->lsq.tracker.at(make_pair(n,this))->addr_resolved=true;      
    
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
  // Workload 
  if(argc != 2)
    assert(false);
  cout << "Path: " << argv[1] << "\n";
  string s(argv[1]);
  string gname = s + "/output/graphOutput.txt";
  string mname = s + "/output/mem.txt";
  string cname = s + "/output/ctrl.txt";
  
  Reader r;
  r.readCfg("sim/config/config.txt", sim.cfg);
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

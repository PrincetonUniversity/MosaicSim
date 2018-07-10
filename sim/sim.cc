//=======================================================================
// Copyright 2018 Princeton University.
//=======================================================================

#include "sim.h"

using namespace apollo;
using namespace std;


int main(int argc, char const *argv[]) {
  Simulator sim;
  // Workload 
  if (argc < 2)
    assert(false);
  cout << "Path: " << argv[1] << "\n";
  string s(argv[1]);
  string gname = s + "/output/graphOutput.txt";
  string mname = s + "/output/mem.txt";
  string cname = s + "/output/ctrl.txt";
  
  // enable verbosity level: -v
  if (argc == 3) {
    string s(argv[2]);
    if ( s == "-v" )
       sim.cfg.vInputLevel = 20;
  }
  else
    sim.cfg.vInputLevel = -1;


  Reader r; 
  r.readCfg("fake_config.txt", sim.cfg);
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
  sim.stats.print();
  return 0;
} 

Context* Context::getNextContext() {
  if(sim->context_list.size() > id+1)
    return sim->context_list.at(id+1);
  else
    return NULL;
}
Context* Context::getPrevContext() {
  if(id != 0)
    return sim->context_list.at(id-1);
  else
    return NULL;
}
void Context::initialize(BasicBlock *bb, Config *cfg, int next_bbid, int prev_bbid) {
  this->bb = bb;
  this->cfg = cfg;
  this->next_bbid = next_bbid;
  this->prev_bbid = prev_bbid;
  live = true;
  // Initialize Context Structures
  for ( int i=0; i<bb->inst.size(); i++ ) {
    Node *n = bb->inst.at(i);
    if(n->typeInstr == ST || n->typeInstr == LD) {
      assert(sim->memory.find(n->id) !=sim->memory.end());
      DynamicNode *d = new DynamicNode(n, this, sim, cfg, sim->memory.at(n->id).front());
      nodes.insert(make_pair(n,d));
      sim->memory.at(n->id).pop();
      sim->lsq.insert(d);
    }
    else
      nodes.insert(make_pair(n, new DynamicNode(n, this, sim, cfg)));  
  }

  if(sim->curr_owner.find(bb->id) == sim->curr_owner.end())
    sim->curr_owner.insert(make_pair(bb->id,this));
  else
    sim->curr_owner.at(bb->id) = this;

  // Initialize External Edges
  for(auto it = nodes.begin(); it!= nodes.end(); ++it) {
    DynamicNode *d = it->second;
    if(d->type == PHI) {
      for(auto pit = d->n->phi_parents.begin(); pit!= d->n->phi_parents.end(); ++pit) {
        Node *src = *pit;
        Context *cc = getPrevContext();
        if(src->bbid == prev_bbid && cc!= NULL) {
          DynamicNode *dsrc = cc->nodes.at(src);
          if(dsrc->completed)
            d->pending_parents--;
        }
      }
    }
    if (d->n->external_parents.size() > 0) {
      for (auto it = d->n->external_parents.begin(); it != d->n->external_parents.end(); ++it) {
        Node *src = *it;
        Context *cc = sim->curr_owner.at(src->bbid);
        DynamicNode *dsrc = cc->nodes.at(src);
        if(dsrc->completed)
          d->pending_external_parents--;
        else
          dsrc->external_dependents.push_back(d);
      }
    }
  }
  for(auto it = nodes.begin(); it!= nodes.end(); ++it)
    it->second->tryActivate();
  if(cfg->vInputLevel > 0)
    cout << "Context [" << id << "]: Created (BB=" << bb->id << ") at cycle " << sim->cycles << "\n";     
}

void Context::process() {
  for (auto it = issue_set.begin(); it!= issue_set.end(); ++it) {
    DynamicNode *d = *it;
    bool res = false;
    if(d->isMem)
      res = d->issueMemNode();
    else
      res = d->issueCompNode();
    if(!res) {
      d->print("Issue Failed", 2);
      next_issue_set.insert(d);
    }
    else {
      if(!d->type == LD)
        active_list.push_back(d);
      d->print("Issue Succesful",1);
    }
  }
  for (int i=0; i < active_list.size(); i++) {
    DynamicNode *d = active_list.at(i);
    if(!d->issued)
      assert(false);
    // Update Remaining Cycles
    if (d->remaining_cycles == 1) {
      d->remaining_cycles--;
      // Check the speculation result for speculative loads
      if (cfg->mem_speculate && d->type == LD && d->speculated) {
        bool exists_unresolved_ST = sim->lsq.exists_unresolved_memop(d, ST);
        if (exists_unresolved_ST) { 
          next_active_list.push_back(d);
          continue;
        }
      }
      nodes_to_complete.push_back(d);
    }
    else if ( d->remaining_cycles > 1) {
      d->remaining_cycles--;
      next_active_list.push_back(d);
      continue;
    }
    else {
      assert(false); 
    }
  }
  issue_set = next_issue_set;
  active_list = next_active_list;
  next_issue_set.clear();
  next_active_list.clear();
}

void Context::complete() {
  for(int i=0; i<nodes_to_complete.size(); i++) {
    DynamicNode *d = nodes_to_complete.at(i);
    d->finishNode();
  }
  nodes_to_complete.clear();
  if (completed_nodes.size() == bb->inst_count) {
    live = false;
    if (cfg->max_active_contexts_BB > 0) {
      sim->outstanding_contexts.at(bb)++;
    }
    if(cfg->vInputLevel > 0)
      cout << "Context [" << id << "] (BB:" << bb->id << ") Finished Execution (Executed " << completed_nodes.size() << " instructions) at cycle " << sim->cycles << "\n";
    sim->stats.num_exec_instr += completed_nodes.size();   // update GLOBAL Stats
    sim->stats.num_finished_context++;
  }
}

void DynamicNode::handleMemoryReturn() {
  print("Memory Transaction Returns", 0);
  print(to_string(outstanding_accesses), 0);

  if(!sim->cfg.ideal_cache)
    sim->cache->fc->cache_insert(addr/64);  //here we ask the functional cache to insert the missing entry
  if(type == LD) {
    if(cfg->mem_speculate) {
      if(outstanding_accesses > 0)
        outstanding_accesses--;
      if(outstanding_accesses != 0) 
        return;
    }
    remaining_cycles = 0;
    // TODO
    c->active_list.push_back(this);
  }
}

bool DynamicNode::issueCompNode() {
  bool canExecute = true;
  // check resource (FU) availability
  if (sim->avail_FUs.at(n->typeInstr) != -1) {
    if (sim->avail_FUs.at(n->typeInstr) == 0)
      canExecute = false;
  }
  if(canExecute) {
    if (sim->avail_FUs.at(n->typeInstr) != -1)
      sim->avail_FUs.at(n->typeInstr)--;
    return true;
  }
  else
    return false;
}

bool DynamicNode::issueMemNode() {
  // Memory Dependency
  issued = true;
  addr_resolved = true;

  bool stallCondition = false;  
  bool canExecute = true;
  bool speculate = false;
  int forwardRes = -1;

  bool exists_unresolved_ST = sim->lsq.exists_unresolved_memop(this, ST);
  bool exists_conflicting_ST = sim->lsq.exists_conflicting_memop(this, ST);
  if (type == ST) {
    bool exists_unresolved_LD = sim->lsq.exists_unresolved_memop(this, LD);
    bool exists_conflicting_LD = sim->lsq.exists_conflicting_memop(this, LD);
    stallCondition = exists_unresolved_ST || exists_conflicting_ST || exists_unresolved_LD || exists_conflicting_LD;
  }
  else if (type == LD) {
    if(cfg->mem_forward)
      forwardRes = sim->lsq.check_forwarding(this);
    if(forwardRes == 1) {
    }
    else if(forwardRes == 0 && cfg->mem_speculate) {
      speculate = true;
    }
    else {
      if(cfg->mem_speculate) {
        stallCondition = exists_conflicting_ST;
        speculate = exists_unresolved_ST && !exists_conflicting_ST;
      }
      else {
        stallCondition = exists_unresolved_ST || exists_conflicting_ST;
      }
      if (type == LD && sim->ports[0] == 0)
        canExecute = false;
      if (type == ST && sim->ports[1] == 0)
        canExecute = false;
    }
  }
  canExecute &= !stallCondition;
  // Issue Successful
  if(canExecute) {
    issued = true;
    speculated = speculate;
    if (type == LD) {
      if(forwardRes == 1) { 
        print("Retrieves Forwarded Data", 1);
        remaining_cycles = 0;
        c->active_list.push_back(this);
      }
      else if(forwardRes == 0 && cfg->mem_speculate) { 
        print("Retrieves Speculatively Forwarded Data", 1);
        remaining_cycles = 0;
        c->active_list.push_back(this);
      }
      else { 
        sim->ports[0]--;
        sim->toMemHierarchy(this);
        if (speculate) {
          outstanding_accesses++;
        }
      }
    }
    else if (n->typeInstr == ST) {
      sim->ports[1]--;
      sim->toMemHierarchy(this);
    }
    return true;
  }
  else
    return false;
}

void DynamicNode::finishNode() {
  print("Finished Execution", 0);
  c->completed_nodes.insert(this);
  completed = true;
  // Handle Resource limits
  if ( sim->avail_FUs.at(n->typeInstr) != -1 ) {
    sim->avail_FUs.at(n->typeInstr)++; 
  }

  // Speculation
  if (cfg->mem_speculate && n->typeInstr == ST) {
    auto misspeculated = sim->lsq.check_speculation(this);
    for(int i=0; i<misspeculated.size(); i++) {
      // Handle Misspeculation
      misspeculated.at(i)->issued = false;
      misspeculated.at(i)->completed = false;
      misspeculated.at(i)->tryActivate();
    }
  }

  if (type == LD || type == ST)         
    speculated = false;

  // Since node <n> ended, update dependents: decrease each dependent's parent count & try to launch each dependent
  set<Node*>::iterator it;
  for (it = n->dependents.begin(); it != n->dependents.end(); ++it) {
    DynamicNode *dst = c->nodes.at(*it);
    if(n->store_addr_dependents.find(*it) != n->store_addr_dependents.end()) {
      dst->addr_resolved = true;
    }
    dst->pending_parents--;
    dst->tryActivate();    
  }

  // The same for external dependents: decrease parent's count & try to launch them
  for(int i=0; i<external_dependents.size(); i++) {
    DynamicNode *dst = external_dependents.at(i);
    if(dst->n->store_addr_dependents.find(n) != dst->n->store_addr_dependents.end()) {
      dst->addr_resolved = true;
    }
    dst->pending_external_parents--;
    dst->tryActivate();
  }
  external_dependents.clear();

  // Same for Phi dependents
  for (it = n->phi_dependents.begin(); it != n->phi_dependents.end(); ++it) {
    Node *dst = *it;
    if(c->next_bbid == dst->bbid) {
      if(Context *cc = c->getNextContext()) {
        cc->nodes.at(dst)->pending_parents--;
        cc->nodes.at(dst)->tryActivate();
      }
    }
  }

  // If node <n> is a TERMINATOR create new context with next bbid in <cf> (a bbid list)
  if (cfg->cf_one_context_at_once && type == TERMINATOR) {
    sim->context_to_create++;
  }
}

void DynamicNode::tryActivate() {
    if(pending_parents > 0 || pending_external_parents > 0) {
//      print("Failed Execution", 0);
      return;
    }
    if(issued || completed)
      assert(false);
    //c->active_list.push_back(this);
    assert(c->issue_set.find(this) == c->issue_set.end());
    c->issue_set.insert(this);
}

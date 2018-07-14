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
  string cfgpath;
  if(argc >= 3) {
    string cfgname(argv[2]);
    cfgpath = "../sim/config/" + cfgname;
  }
  else 
    cfgpath = "../sim/config/default.txt";

  string gname = s + "/output/graphOutput.txt";
  string mname = s + "/output/mem.txt";
  string cname = s + "/output/ctrl.txt";
  
  // enable verbosity level: -v
  if (argc == 4) {
    string s(argv[3]);
    if ( s == "-v" )
       sim.cfg.vInputLevel = 20;
  }
  else
    sim.cfg.vInputLevel = -1;


  Reader r; 
  r.readCfg(cfgpath, sim.cfg);
  r.readGraph(gname, sim.g, sim.cfg);
  r.readProfMemory(mname , sim.memory);
  r.readProfCF(cname, sim.cf);
  sim.g.inductionOptimization();
  
  sim.initialize();
  cout << "Initialization Complete \n";
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
void Context::insertQ(DynamicNode *d) {
  if(d->n->lat > 0)
    pq.push(make_pair(d, sim->cycles+d->n->lat-1));
  else
    pq.push(make_pair(d, sim->cycles));
}
void Context::initialize(BasicBlock *bb, Config *cfg, int next_bbid, int prev_bbid) {
  this->bb = bb;
  this->cfg = cfg;
  this->next_bbid = next_bbid;
  this->prev_bbid = prev_bbid;  
  live = true;
  // Initialize Context Structures
  for ( unsigned int i=0; i<bb->inst.size(); i++ ) {
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
    assert(!d->issued);
    if(d->isMem)
      res = d->issueMemNode();
    else
      res = d->issueCompNode();
    if(!res) {
      d->print("Issue Failed", 2);
      next_issue_set.insert(d);
    }
    else {
      if(d->type != LD)
        insertQ(d);
      d->print("Issue Succesful",1);
    }
  }
  for(auto it = speculated_set.begin(); it!= speculated_set.end();) {
    DynamicNode *d = *it;
    if(cfg->mem_speculate && d->type == LD && d->speculated && sim->lsq.check_unresolved_store(d)) {
      sim->stats.num_mem_hold++;
      ++it;
    }
    else {
      nodes_to_complete.insert(d);
      it = speculated_set.erase(it);
    }
  }
  while(pq.size() > 0) {
    if(pq.top().second > sim->cycles)
      break;
    DynamicNode *d = pq.top().first;
    nodes_to_complete.insert(d);
    pq.pop();
  }

  issue_set = move(next_issue_set);
  next_issue_set.clear();
}

void Context::complete() {
  for(auto it = nodes_to_complete.begin(); it!= nodes_to_complete.end(); ++it) {
    DynamicNode *d =*it;
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
  if(type == LD) {
    if(cfg->mem_speculate) {
      if(outstanding_accesses > 0)
        outstanding_accesses--;
      if(outstanding_accesses != 0) 
        return;
    }
    c->insertQ(this);
  }
}

bool DynamicNode::issueCompNode() {
  issued = true;
  sim->stats.num_comp_issue_try++;
  // check resource (FU) availability
  if (sim->avail_FUs.at(n->typeInstr) != -1) {
    if (sim->avail_FUs.at(n->typeInstr) == 0)
      return false;
  }
  if (sim->avail_FUs.at(n->typeInstr) != -1)
    sim->avail_FUs.at(n->typeInstr)--;
  sim->stats.num_comp_issue_pass++;
  return true;
}

bool DynamicNode::issueMemNode() {

  bool speculate = false;
  int forwardRes = -1;
  if(type == LD)
    sim->stats.num_mem_load_try++;
  else
    sim->stats.num_mem_store_try++;
  sim->stats.num_mem_issue_try++;
  if(sim->ports[0] == 0 && type == LD) {
    sim->stats.memory_events[6]++;
    return false;
  }
  if(sim->ports[1] == 0 && type == ST) {
    sim->stats.memory_events[7]++;
    return false;
  }
  if(type == LD && cfg->mem_forward) {
    forwardRes = sim->lsq.check_forwarding(this);
    if(forwardRes == 0 && cfg->mem_speculate) {
      sim->stats.num_speculated_forwarded++;
      speculate = true;
    }
    else if(forwardRes == 1)
      sim->stats.num_forwarded++;
  }
  if(type == LD && forwardRes == -1) {
    int res = sim->lsq.check_load_issue(this, cfg->mem_speculate);
    if(res == 0) {
      sim->stats.num_speculated++;
      speculate = true;
    }
    else if(res == -1) {
      sim->stats.memory_events[0]++;
      return false;
    }
  }
  else if(type == ST) {
    if(!sim->lsq.check_store_issue(this)) {
      sim->stats.memory_events[1]++;
      return false;
    }
  }
  
  issued = true;
  if(type == LD)
    sim->stats.num_mem_load_pass++;
  else
    sim->stats.num_mem_store_pass++;
  sim->stats.num_mem_issue_pass++;
  speculated = speculate;
  if (type == LD) {
    if(forwardRes == 1) { 
      print("Retrieves Forwarded Data", 1);
      c->insertQ(this);
    }
    else if(forwardRes == 0 && cfg->mem_speculate) { 
      print("Retrieves Speculatively Forwarded Data", 1);
      c->speculated_set.insert(this);
    }
    else {
      sim->ports[0]--;
      sim->toMemHierarchy(this);
      if (speculate) {
        outstanding_accesses++;
      }
    }
  }
  else if (type == ST) {
    sim->ports[1]--;
    sim->toMemHierarchy(this);
  }
  return true;
}

void DynamicNode::finishNode() {
  print("Finished Execution", 0);
  c->completed_nodes.insert(this);
  completed = true;
  // Handle Resource limits
  if ( sim->avail_FUs.at(n->typeInstr) != -1 )
    sim->avail_FUs.at(n->typeInstr)++; 

  // Speculation
  if (cfg->mem_speculate && n->typeInstr == ST) {
    auto misspeculated = sim->lsq.check_speculation(this);
    for(unsigned int i=0; i<misspeculated.size(); i++) {
      // Handle Misspeculation
      sim->stats.num_misspec++;
      misspeculated.at(i)->issued = false;
      misspeculated.at(i)->completed = false;
      misspeculated.at(i)->tryActivate();
    }
  }

  if(isMem)       
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
  for(unsigned int i=0; i<external_dependents.size(); i++) {
    DynamicNode *dst = external_dependents.at(i);
    if(n->store_addr_dependents.find(n) != n->store_addr_dependents.end()) {
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
  // if (cfg->cf_one_context_at_once && type == TERMINATOR) {
  //   sim->context_to_create++;
  // }
}

void DynamicNode::tryActivate() {
    if(pending_parents > 0 || pending_external_parents > 0) {
      return;
    }
    if(issued || completed)
      assert(false);
    if(type == TERMINATOR) {
      c->completed_nodes.insert(this);
      completed = true;
      if (cfg->cf_mode == 0 && type == TERMINATOR)
        sim->context_to_create++;
    }
    else {
      assert(c->issue_set.find(this) == c->issue_set.end());
      if(isMem)
        addr_resolved = true;
      c->issue_set.insert(this);
    }
}

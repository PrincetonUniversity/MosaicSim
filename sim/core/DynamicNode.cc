#include "DynamicNode.h"
#include "Core.h"
#include "../memsys/Cache.h"
//#include "../sim.h"
using namespace std;

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
ostream& operator<<(ostream &os, Context &c) {
  os << "[Context-" << c.id <<"] ";
  return os;
}
void Context::print(string str, int level) {
  if( level < cfg.verbLevel )
    cout << (*this) << str << "\n";
}
void Context::insertQ(DynamicNode *d) {
  if(d->n->lat > 0)
    pq.push(make_pair(d, sim->cycles+d->n->lat-1));
  else
    pq.push(make_pair(d, sim->cycles));
}
void Context::initialize(BasicBlock *bb, int next_bbid, int prev_bbid) {
  this->bb = bb;
  this->next_bbid = next_bbid;
  this->prev_bbid = prev_bbid;  
  live = true;
  // Initialize Context Structures
  for ( unsigned int i=0; i<bb->inst.size(); i++ ) {
    Node *n = bb->inst.at(i);
    if(n->typeInstr == ST || n->typeInstr == LD) {
      assert(sim->memory.find(n->id) !=sim->memory.end());
      DynamicNode *d = new DynamicNode(n, this, sim, sim->memory.at(n->id).front());
      nodes.insert(make_pair(n,d));
      sim->memory.at(n->id).pop();
      sim->lsq.insert(d);
    }
    else
      nodes.insert(make_pair(n, new DynamicNode(n, this, sim)));  
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

  print("Created (BB:" + to_string(bb->id) + ") " , 0);
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
      d->print("Issue Failed", 0);
      next_issue_set.insert(d);
    }
    else {
      if(d->type != LD)
        insertQ(d);
      d->print("Issue Succesful",0);
    }
  }
  for(auto it = speculated_set.begin(); it!= speculated_set.end();) {
    DynamicNode *d = *it;
    if(cfg.mem_speculate && d->type == LD && d->speculated && sim->lsq.check_unresolved_store(d)) {
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
  for(auto it = nodes_to_complete.begin(); it != nodes_to_complete.end(); ++it) {
    DynamicNode *d =*it;
    d->finishNode();
  }
  nodes_to_complete.clear();
  if (completed_nodes.size() == bb->inst_count) {
    live = false;
    if (cfg.max_active_contexts_BB > 0) {
      sim->outstanding_contexts.at(bb)++;
    }
    print("Finished (BB:" + to_string(bb->id) + ") ", 0);
    // update GLOBAL Stats
    stat.update("contexts");
    stat.update("total_instructions", completed_nodes.size());
    // update LOCAL Stats
    sim->local_stat.update("contexts");
    sim->local_stat.update("total_instructions", completed_nodes.size());

    // Update activity counters
    int word_size_bytes = 4;  // TODO: allow different sizes. Now, word_size is a constant
    for(auto it = completed_nodes.begin(); it != completed_nodes.end(); ++it) {
      DynamicNode *d =*it;
      if (d->type == LD) 
        sim->activity_mem.bytes_read += word_size_bytes;     
      else if (d->type == ST) 
        sim->activity_mem.bytes_write += word_size_bytes;
      sim->activity_FUs.at(d->type)++;
    }
  }
}

DynamicNode::DynamicNode(Node *n, Context *c, Core *sim, uint64_t addr) : n(n), c(c), sim(sim) {
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
bool DynamicNode::operator== (const DynamicNode &in) {
  if(c->id == in.c->id && n->id == in.n->id)
    return true;
  else
    return false;
}
bool DynamicNode::operator< (const DynamicNode &in) const {
  if(c->id < in.c->id) 
    return true;
  else if(c->id == in.c->id && n->id < in.n->id)
    return true;
  else
    return false;
}
ostream& operator<<(ostream &os, DynamicNode &d) {
  os << "[Context-" <<d.c->id <<"] Node - " << d.n->name <<" ";
  return os;
}
void DynamicNode::print(string str, int level) {
  if( level < cfg.verbLevel )
    cout << (*this) << str << "\n";
}

void DynamicNode::handleMemoryReturn() {
  print("Memory Data Ready", 1);
  print(to_string(outstanding_accesses), 1);
  if(type == LD) {
    if(cfg.mem_speculate) {
      if(outstanding_accesses > 0)
        outstanding_accesses--;
      if(outstanding_accesses != 0) 
        return;
    }
    c->insertQ(this);
  }
}

void DynamicNode::tryActivate() {
    if(pending_parents > 0 || pending_external_parents > 0)
      return;
    if(issued || completed)
      assert(false);
    if(type == TERMINATOR) {
      c->completed_nodes.insert(this);
      completed = true;
      if (cfg.cf_mode == 0 && type == TERMINATOR)
        sim->context_to_create++;
    }
    else {
      assert(c->issue_set.find(this) == c->issue_set.end());
      if(isMem) {
        addr_resolved = true;
        sim->lsq.resolveAddress(this);
      }
      c->issue_set.insert(this);
    }
}

bool DynamicNode::issueCompNode() {
  // TJH: The following code is a hack
  /*if (n->isComp()) {
    if(sim->inputQ.empty()) {      
      return false;
    }
    else {
      
      sim->inputQ.pop();
      cout << "Received " << n->name << " " << n->id << " In Context: " << c->id << endl;;
      return true;
    }    
  } 
  
  if (n->isSupp()) {
    sim->digestor->intercon->insert(this);
    cout << "Supplied " << n->name <<" " << n->id << " In Context: " << c->id << endl;
    return true;
  }*/
  
  stat.update("comp_issue_try");
  sim->local_stat.update("comp_issue_try");
  // check for resource (FU) availability
  if (sim->available_FUs.at(n->typeInstr) != -1) {
    if (sim->available_FUs.at(n->typeInstr) == 0)
      return false;
    else
      sim->available_FUs.at(n->typeInstr)--;
  }
  stat.update("comp_issue_success");
  sim->local_stat.update("comp_issue_success");
  issued = true;
  return true;
}

bool DynamicNode::issueMemNode() {
  bool speculate = false;
  int forwardRes = -1;
  if(type == LD) {
    stat.update("load_issue_try");
    sim->local_stat.update("load_issue_try");
  }
  else {
    stat.update("store_issue_try");
    sim->local_stat.update("store_issue_try");
  }
  // check for enough resources (memory ports)
  if(type == LD && sim->cache->ports[0] == 0 )
    return false;
  if(type == ST && sim->cache->ports[1] == 0 )
    return false;
  if(type == LD && cfg.mem_forward) {
    forwardRes = sim->lsq.check_forwarding(this);
    if(forwardRes == 0 && cfg.mem_speculate) {
      stat.update("speculatively_forwarded");
      sim->local_stat.update("speculatively_forwarded");
      speculate = true;
    }
    else if(forwardRes == 1) {
      stat.update("forwarded");
      sim->local_stat.update("forwarded");
    }
  }
  if(type == LD && forwardRes == -1) {
    int res = sim->lsq.check_load_issue(this, cfg.mem_speculate);
    if(res == 0) {
      stat.update("speculated");
      sim->local_stat.update("speculated");
      speculate = true;
    }
    else if(res == -1)
      return false;
  }
  else if(type == ST) {
    if(!sim->lsq.check_store_issue(this))
      return false;
  }
  // at this point the memory request will be issued for sure
  issued = true;
  if(type == LD) {
    stat.update("load_issue_success");
    sim->local_stat.update("load_issue_success");
  }
  else {
    stat.update("store_issue_success");
    sim->local_stat.update("store_issue_success");
  }
  speculated = speculate;
  if (type == LD) {
    if(forwardRes == 1) { 
      print("Retrieves Forwarded Data", 1);
      c->insertQ(this);
    }
    else if(forwardRes == 0 && cfg.mem_speculate) { 
      print("Retrieves Speculatively Forwarded Data", 1);
      c->speculated_set.insert(this);
    }
    else {
      print("Access Memory Hierarchy", 1);
      sim->cache->ports[0]--;
      sim->toMemHierarchy(this);
      if (speculate) {
        outstanding_accesses++;
      }
    }
  }
  else if (type == ST) {
    print("Access Memory Hierarchy", 1);
    sim->cache->ports[1]--;
    sim->toMemHierarchy(this);
  }
  return true;
}

void DynamicNode::finishNode() {
  print("Finished Execution", 1);
  c->completed_nodes.insert(this);
  completed = true;

  // Handle Resource limits
  if ( sim->available_FUs.at(n->typeInstr) != -1 )
    sim->available_FUs.at(n->typeInstr)++; 

  // Speculation
  if (cfg.mem_speculate && n->typeInstr == ST) {
    auto misspeculated = sim->lsq.check_speculation(this);
    for(unsigned int i=0; i<misspeculated.size(); i++) {
      // Handle Misspeculation
      stat.update("misspeculated");
      sim->local_stat.update("misspeculated");
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
      sim->lsq.resolveAddress(dst);
    }
    dst->pending_parents--;
    dst->tryActivate();
  }

  // The same for external dependents: decrease parent's count & try to launch them
  for(unsigned int i=0; i<external_dependents.size(); i++) {
    DynamicNode *dst = external_dependents.at(i);
    if(n->store_addr_dependents.find(n) != n->store_addr_dependents.end()) {
      dst->addr_resolved = true;
      sim->lsq.resolveAddress(dst);
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
}

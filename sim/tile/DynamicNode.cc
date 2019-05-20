#include "DynamicNode.h"
#include "Core.h"
#include "../memsys/Cache.h"
using namespace std;

string load_issue_try="load_issue_try";
string load_issue_success = "load_issue_success";
string store_issue_try= "store_issue_try";
string store_issue_success = "store_issue_success";
string comp_issue_try="comp_issue_try";
string comp_issue_success="comp_issue_success";
string send_issue_try="send_issue_try";
string send_issue_success="send_issue_success";
string recv_issue_try="recv_issue_try";
string recv_issue_success="recv_issue_success";
string stval_issue_try="stval_issue_try";
string stval_issue_success="stval_issue_success";
string staddr_issue_try="staddr_issue_try";
string staddr_issue_success="staddr_issue_success";
string ld_prod_issue_try="ld_prod_issue_try";
string ld_prod_issue_success="ld_prod_issue_success";    
string lsq_insert_success="lsq_insert_success";
string lsq_insert_fail="lsq_insert_fail";
string total_instructions="total_instructions";
string Issue_Failed="Issue_Failed";
string Issue_Succesful="Issue_Succesful";
string bytes_read="bytes_read";
string bytes_write="bytes_write";
string Access_Memory_Hierarchy="Access_Memory_Hierarchy";
string Memory_Data_Ready="Memory_Data_Ready";

Context* Context::getNextContext() {
  if(core->context_list.size() > id+1)
    return core->context_list.at(id+1);
  else
    return NULL;
}
Context* Context::getPrevContext() {
  if(id != 0)
    return core->context_list.at(id-1);
  else
    return NULL;
}
ostream& operator<<(ostream &os, Context &c) {
  os << "[Context-" << c.id <<"] ";
  return os;
}
void Context::print(string str, int level) {
  if( level < cfg.verbLevel )
    cout << (*this) << str << endl;
}
void Context::insertQ(DynamicNode *d) {
  if(d->n->lat > 0) {
    //pq.push(make_pair(d, core->cycles+d->n->lat-1));
    pq.push(make_pair(d, core->cycles+d->n->lat));
  }
  else {
    pq.push(make_pair(d, core->cycles));
  }
}
void Context::initialize(BasicBlock *bb, int next_bbid, int prev_bbid) {
  this->bb = bb;
  this->next_bbid = next_bbid;
  this->prev_bbid = prev_bbid;  
  live = true;
  // Initialize Context Structures
  for ( unsigned int i=0; i<bb->inst.size(); i++ ) {
    Node *n = bb->inst.at(i);
    DynamicNode* d;

    if(n->typeInstr == ST || n->typeInstr == LD || n->typeInstr == STADDR ||  n->typeInstr == LD_PROD) {
      if(core->memory.find(n->id)==core->memory.end()) {
        cout << "Assertion about to fail for: " << n->name << endl;         }

      assert(core->memory.find(n->id)!=core->memory.end());
      d = new DynamicNode(n, this, core, core->memory.at(n->id).front());
      
      nodes.insert(make_pair(n,d));
      core->memory.at(n->id).pop();

      if(n->typeInstr == ST || n->typeInstr == LD || n->typeInstr == STADDR) {
        core->lsq.insert(d);
      }
    }
    else {
      d = new DynamicNode(n, this, core); 
      nodes.insert(make_pair(n, d));          
    }
    d->width=n->width;
    if(d->n->typeInstr==BS_VECTOR_INC) {
      core->sim->load_count+=d->width;
    }
    if(d->isDESC) {        
      core->sim->orderDESC(d);
    }
    core->window.insertDN(d); //this helps maitain a program-order ordering of all instructions
  }

  if(core->curr_owner.find(bb->id) == core->curr_owner.end())
    core->curr_owner.insert(make_pair(bb->id,this));
  else
    core->curr_owner.at(bb->id) = this;

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
        Context *cc = core->curr_owner.at(src->bbid);
        DynamicNode *dsrc = cc->nodes.at(src);
        if(dsrc->completed)
          d->pending_external_parents--;
        else
          dsrc->external_dependents.push_back(d);
      }
    }
  }
  for(auto it = nodes.begin(); it!= nodes.end(); ++it) {
    assert(!it->second->issued && !it->second->completed);
    DynamicNode* d=it->second;
    //correctly predict all branches
    if(core->local_cfg.cf_mode==1 && d->type==TERMINATOR) {
      d->pending_parents=0;
      d->pending_external_parents=0;
    }
    //here, if branch prediction is enabled we're going to launch the terminator instruction, which will in turn launch the next contexts upon completion of finishNode()
    //if there's a misprediction, we will add a penalty latency, which delays the completion of the terminator after issue, which in turn delays the launching of the next context 
    if (d->type == TERMINATOR && core->local_cfg.branch_prediction) {
      //if branch predictor mispredicts, add penalty 
      if(!core->predict_branch(d)) {
        d->n->lat+=core->local_cfg.misprediction_penalty; //delay completion, which delays launch of next context
      }
    }
    //if it's a TERMINATOR with branch prediction mode, it'll just get activated
    d->tryActivate(); 
  }
  print("Created (BB:" + to_string(bb->id) + ") " , 5);
}

void DynamicNode::register_issue_try() {
  if(type == SEND) {
    stat.update(send_issue_try);
    core->local_stat.update(send_issue_try);
  }  
  else if(type == RECV) {
    stat.update(recv_issue_try);
    core->local_stat.update(recv_issue_try);
  }
  else if(type == STADDR) {
    stat.update(staddr_issue_try);
    core->local_stat.update(staddr_issue_try);
  }
  else if(type == STVAL) {
    stat.update(stval_issue_try);
    core->local_stat.update(stval_issue_try);
  }
  else if(type == LD_PROD) {    
    stat.update(ld_prod_issue_try);
    core->local_stat.update(ld_prod_issue_try);
  }
  else if(type == LD) {
    stat.update(load_issue_try);
    core->local_stat.update(load_issue_try);
  }
  else if (type == ST) {
    stat.update(store_issue_try);
    core->local_stat.update(store_issue_try);
  }
  else {
    stat.update(comp_issue_try);
    core->local_stat.update(comp_issue_try);
  }
}

void DynamicNode::register_issue_success() {
  if(type == SEND) {
    stat.update(send_issue_success);
    core->local_stat.update(send_issue_success);
  }  
  else if(type == RECV) {
    stat.update(recv_issue_success);
    core->local_stat.update(recv_issue_success);
  }
  else if(type == STADDR) {
    stat.update(staddr_issue_success);
    core->local_stat.update(staddr_issue_success);
  }
  else if(type == STVAL) {
    stat.update(stval_issue_success);
    core->local_stat.update(stval_issue_success);
  }
  else if(type == LD_PROD) {    
    stat.update(ld_prod_issue_success);
    core->local_stat.update(ld_prod_issue_success);
  }
  else if(type == LD) {
    stat.update(load_issue_success);
    core->local_stat.update(load_issue_success);
  }
  else if (type == ST) {
    stat.update(store_issue_success);
    core->local_stat.update(store_issue_success);
  }
  else {
    stat.update(comp_issue_success);
    core->local_stat.update(comp_issue_success);
  }
}

uint64_t trans_id=0;

void Context::process() {
  bool window_full=false;
  bool issue_stats_mode=false; 

  for (auto it = issue_set.begin(); it!= issue_set.end();) {
   
    DynamicNode *d = *it;
    
    if(window_full) {     
      if (issue_stats_mode) {
        d->register_issue_try();
        d->print(Issue_Failed, 5);
        next_issue_set.insert(d);
        ++it;
        continue;
      }
      else {
        break;
      }      
    }    

    bool window_available = core->window.canIssue(d);
    window_full=!window_available;
    bool res = window_available;
    assert(!d->issued);
    /*
    if(d->type == TERMINATOR && res) {    
      d->c->completed_nodes.insert(d);
      //d->completed = true;
      d->issued = true;
      if (core->local_cfg.cf_mode == 0 && d->type == TERMINATOR)
        core->context_to_create++;     
    }
    */
    if(d->isMem)
      res = res && d->issueMemNode();
    else if (d->isDESC) {     
      res = res && d->issueDESCNode();
      //depends on lazy eval, as descq will insert if *its* resources are available
    }       
    else {
      res = res && d->issueCompNode(); //depends on lazy eval
    }
    if(!res) {
      if(issue_stats_mode) {
        next_issue_set.insert(d);
      }
      ++it;
    }
    else {
      core->window.issue();
      d->issued=true;
      if(cfg.verbLevel >= 5) {
        cout << "Cycle: " << core->cycles << " \n"; 
        d->print("Issued", 5);
      }

  
      if(d->type != LD && !d->isDESC && d->type!=BARRIER) { //DESC instructions are completed by desq and BARRIER instrs by the sim barrier object
        insertQ(d);        
      }
      if(issue_stats_mode) {
        d->register_issue_success();
        ++it;
      }
      else {
        it=issue_set.erase(it); //gets the next iteration
      }
    }
  }
  for(auto it = speculated_set.begin(); it!= speculated_set.end();) {
    DynamicNode *d = *it;
    if(core->local_cfg.mem_speculate && (d->type == LD || d->type == LD_PROD) && d->speculated && core->lsq.check_unresolved_store(d)) { //added ld_prod here even though we don't currently speculate with it 
      ++it;
    }
    else {
      nodes_to_complete.insert(d);      
      it = speculated_set.erase(it);
    }
  }
  while(pq.size() > 0) {
    if(pq.top().second > core->cycles)
      break;
    DynamicNode *d = pq.top().first;
    nodes_to_complete.insert(d);
    pq.pop();
  }
  if(issue_stats_mode) {
    issue_set = move(next_issue_set);
    next_issue_set.clear();
  }
}

void Context::complete() {
  for(auto it = nodes_to_complete.begin(); it != nodes_to_complete.end(); ++it) {
    DynamicNode *d =*it;
    d->finishNode();  
  }
  
  nodes_to_complete.clear();
  if (completed_nodes.size() == bb->inst_count) {
    live = false;   
    if (core->local_cfg.max_active_contexts_BB > 0) {      
      core->outstanding_contexts.at(bb)++;      
    }
    print("Finished (BB:" + to_string(bb->id) + ") ", 5);
    // update GLOBAL Stats    
    stat.update("contexts");
    stat.update(total_instructions, completed_nodes.size());
    // update LOCAL Stats
    core->local_stat.update("contexts");
    core->local_stat.update(total_instructions, completed_nodes.size());

    // Update activity counters
    for(auto it = completed_nodes.begin(); it != completed_nodes.end(); ++it) {
      DynamicNode *d =*it;      
      
      if (d->type == LD || d->type == LD_PROD) {
        stat.update(bytes_read, word_size_bytes);
        core->local_stat.update(bytes_read, word_size_bytes);
      }
      else if (d->type == ST || d->type == STADDR) {
        stat.update(bytes_write, word_size_bytes);
        core->local_stat.update(bytes_write, word_size_bytes);
      }
      stat.update(core->instrToStr(d->type));
      core->local_stat.update(core->instrToStr(d->type));
      
      //delete d; //JLA
    }
  }
}

DynamicNode::DynamicNode(Node *n, Context *c, Core *core, uint64_t addr) : n(n), c(c), core(core) {
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
  
  if(n->typeInstr==SEND || n->typeInstr==RECV || n->typeInstr==STADDR || n->typeInstr==STVAL || n->typeInstr==LD_PROD)
    isDESC = true;
  else
    isDESC = false;
  
  if(addr == 0 || isDESC)
    isMem = false;
  else
    isMem = true;
}

bool DynamicNode::operator== (const DynamicNode &in) {  
  if(c->id == in.c->id && n->id == in.n->id)
    return true;
  else
    return false;
  
  /*
  try{
    if(c->id == in.c->id && n->id == in.n->id)
      return true;
    else
      return false;
  }
  catch(...) {
    cout << "Failing nodes: ";
    DynamicNode orig=*this;
    orig.print("failing node", 5);
    DynamicNode d=in;
    d.print("failing node arg", 5);
    assert(false);
  }
  */
}
bool DynamicNode::operator< (const DynamicNode &in) const {
  /* if(c->id < in.c->id) 
    return true;
  else if(c->id == in.c->id && n->id < in.n->id)
    return true;
  else
  return false;
  */
  /*  cout << "before sf \n";
  if (n->id) { //< in.n->id) {
    cout<<"";
  }
  cout << "after sf \n";
  */
  //DynamicNode orig=*this;
  //orig.print("failing node", 5);
  //DynamicNode d=in;
  //d.print("failing node arg", 5);
  if(c->id < in.c->id) 
    return true;
  if(c->id == in.c->id && n->id < in.n->id)
    return true;
  else
    return false;
/*  
  try {
    if(c->id < in.c->id) 
      return true;
    else if(c->id == in.c->id && n->id < in.n->id)
      return true;
    else
      return false; 
  }
  catch(...) {
    cout << "Failing nodes: ";
    DynamicNode orig=*this;
    orig.print("failing node", 5);
    DynamicNode d=in;
    d.print("failing node arg", 5);
    assert(false);
  }
*/ 
}
ostream& operator<<(ostream &os, DynamicNode &d) {
  string descid;
  if (d.n->typeInstr==SEND || d.n->typeInstr==RECV || d.n->typeInstr==STVAL || d.n->typeInstr==STADDR ||  d.n->typeInstr==LD_PROD) {
    descid=" [DESC ID: " + to_string(d.desc_id) + "]";
  }
  os << "[Core: " <<d.core->id << "] [Context: " <<d.c->id << "]" << descid << " [Node: " << d.n->id << "] [Instruction: " << d.n->name <<"] ";
  return os;
}
void DynamicNode::print(string str, int level) {
  if( level < cfg.verbLevel )
    cout << (*this) << str << endl;
}
void DynamicNode::handleMemoryReturn() {
      //update load latency stats
  if(type==LD || type==LD_PROD) {
    assert(core->sim->load_stats_map.find(this)!=core->sim->load_stats_map.end());
    long long current_cycle = core->cycles;
    auto& entry_tuple=core->sim->load_stats_map[this];
    get<1>(entry_tuple)=current_cycle;
  }
  print(Memory_Data_Ready, 5);
  print(to_string(outstanding_accesses), 5);
  if(type == LD || type == LD_PROD) {
    if(core->local_cfg.mem_speculate) {
      if(outstanding_accesses > 0)
        outstanding_accesses--;
      if(outstanding_accesses != 0) 
        return;
    }
    if(type == LD) {
      c->insertQ(this);
    }
    //luwa desc modification
    if(type == LD_PROD) {
      
      //here tell descq to move       
      mem_status=RETURNED; //this signals DESCQ, it can move it from term ld buff to commQ and finally complete
    }
  }
}

void DynamicNode::tryActivate() {
  //if cf_mode==1 or branch prediction is on, we immidiately set dependents to 0 and call tryActivate() to issue terminator immediately; this means the parents of terminators will still call tryactivate on terminator, which we don't want to re-issue
  bool must_wait_for_parents = (type!=TERMINATOR || (core->local_cfg.cf_mode==0 && !core->local_cfg.branch_prediction));
  
  if(must_wait_for_parents && (pending_parents > 0 || pending_external_parents > 0)) {  //not ready to activate
    return;
  }

  //don't activate multiple times when branch prediction is on
  if(!must_wait_for_parents && issued) {
    return;
  }

  if(issued || completed) {
    this->print("trying to activate issued or completed instruction",5);
    assert(false);
  }
  /*  if(type == TERMINATOR) {    
      c->completed_nodes.insert(this);
      issued = true;
      completed = true;
      if (core->local_cfg.cf_mode == 0 && type == TERMINATOR)
      core->context_to_create++;       
      }*/
  //else {
  
  //if you don't need to wait for parents, 2 parents can tryActivate() in same cycle, which 
  if(must_wait_for_parents && c->issue_set.find(this) != c->issue_set.end()) {
    print("should not already be issued" , 5);
    assert(false);
  }
  
  if(isMem || type==STADDR) { 
    addr_resolved = true;
    core->lsq.resolveAddress(this);
  }
  c->issue_set.insert(this);
}

bool DynamicNode::issueCompNode() { 
  // check for resource (FU) availability
  if (core->available_FUs.at(n->typeInstr) != -1) {
    if (core->available_FUs.at(n->typeInstr) == 0)
      return false;
    else
      core->available_FUs.at(n->typeInstr)--;
  }
  bool can_issue=true;
  //free up all barriers or register this one
  if(type == BARRIER) {    
    can_issue=core->sim->barrier->register_barrier(this);
  }
  
  if(can_issue) {
    stat.update(comp_issue_success);
    core->local_stat.update(comp_issue_success);
  }
  //issued = true;
  return can_issue;
}

bool DynamicNode::issueMemNode() {
  
  bool speculate = false;
  int forwardRes = -1; 
  // FIXME
  if(!core->canAccess(type == LD)) { 
    return false;
  }
  
  DESCQ* descq=core->sim->get_descq(this);
  
  //check DESCQ's Store Address Buffer for conflicts
  if(type==LD && (descq->sab_has_dependency(this)!=NULL)) { //sab_has_dependency() returns null if nothing found
    return false; 
  }
  if(type == LD && core->local_cfg.mem_forward) {
    forwardRes = core->lsq.check_forwarding(this);
    if(forwardRes == 0 && core->local_cfg.mem_speculate) {
      stat.update("speculatively_forwarded");
      core->local_stat.update("speculatively_forwarded");
      speculate = true;
    }
    else if(forwardRes == 1) {
      stat.update("forwarded");
      core->local_stat.update("forwarded");
    }
  }
  
  if(type == LD && forwardRes == -1) {
    int res = core->lsq.check_load_issue(this, core->local_cfg.mem_speculate);
  
    if(res == 0) {
      stat.update("speculated");
      core->local_stat.update("speculated");
      speculate = true;
    }
    else if(res == -1)
      return false;
  }
  else if(type == ST) {
    if(!core->lsq.check_store_issue(this))
      return false;
  }
  // at this point the memory request will be issued for sure
  //issued = true;
  if(type == LD) {
    stat.update(load_issue_success);
    core->local_stat.update(load_issue_success);
  }
  else {
    stat.update(store_issue_success);
    core->local_stat.update(store_issue_success);
  }
  speculated = speculate;
  if (type == LD) {
    if(forwardRes == 1) { 
      print("Retrieves Forwarded Data", 5);
      c->insertQ(this);
    }
    else if(forwardRes == 0 && core->local_cfg.mem_speculate) { 
      print("Retrieves Speculatively Forwarded Data", 5);
      c->speculated_set.insert(this);
    }
    else {
      print(Access_Memory_Hierarchy, 5);
      core->access(this);  //send to mem hierarchy
      if (speculate) {
        outstanding_accesses++;
      }
    }
  }
  else if (type == ST) {
    print(Access_Memory_Hierarchy, 5);
    core->access(this); //send to mem hierarchy
  }
  return true;
}

int stl_fwd_count=0;
int lsq_fwd_count=0;
bool DynamicNode::issueDESCNode() {
  bool can_issue=true;
 
  //luwa rob fix
  /*if(type == STADDR) {
    if(!core->lsq.check_store_issue(this)) {
      can_issue = false; 
    }
  }
  */
  DESCQ* descq=core->sim->get_descq(this);
  int lsq_result=0;
  bool can_forward_from_lsq = false;
  bool can_forward_from_svb = false;
  DynamicNode* forwarding_staddr=NULL;
  if(type == LD_PROD) {
    can_forward_from_lsq = core->local_cfg.mem_forward && (core->lsq.check_forwarding(this)==1);

      //0 means can forward speculatively from lsq
      //1 means we found a closest older STORE (not STADDR) in lsq that has completed, no speculation
   
    lsq_result = core->lsq.check_load_issue(this, false);
    // -1 can't issue at all, no matter what
    //0 can issue with speculation (perhaps unresolved matching older store)
    //1 is can definitely issue (nothing uncompleted or unresolved above)
    //2 is there's an issued staddr above, can *possibly* do stl fwding
    can_issue=can_issue && lsq_result!=-1 && lsq_result!=0;

    forwarding_staddr = descq->sab_has_dependency(this);
    
   
    can_forward_from_svb = forwarding_staddr!=NULL && forwarding_staddr!=descq->SAB.begin()->second; /*&& descq->recv_map.find(desc_id)!=descq->recv_map.end() && descq->recv_map[desc_id]->issued*/;
    //tells us there's a matching staddr that can bypass and forward store value
    //other condition avoids deadlock due to super young recv not getting a chance to enter RoB to get value from stval because a lot of instructions already fill RoB 

    //here ld_prod must go to memory, check if mem system can accept
    if(!can_forward_from_lsq && !can_forward_from_svb) {
      mem_status=PENDING; //must issue mem request below
      can_issue=can_issue && core->canAccess(true);
     
    }
    else if(can_forward_from_lsq) {
      mem_status=LSQ_FWD;      
    }
    else if(can_forward_from_svb) {
      mem_status=DESC_FWD;   
    }
  }

  //insert into the desc structures if there's space, insert() returns false otherwise
  //note: relies on lazy evaluation of booleans, must call insert() last
  can_issue=can_issue && descq->insert(this,forwarding_staddr);
  
  if(!can_issue) {
    mem_status=NONE; //reset mem status 
  }
  
  if(can_issue) {
    //collect stats on runahead distance
    if(type==LD_PROD || type==SEND) {
      if(descq->send_runahead_map.find(desc_id)!=descq->send_runahead_map.end()) {
        descq->send_runahead_map[desc_id] =  descq->send_runahead_map[desc_id] - core->cycles; //runahead will be negative here because recv finished first
      }
      else {
        descq->send_runahead_map[desc_id] = core->cycles;
      }
    }
    if(type==RECV) {
      if(descq->send_runahead_map.find(desc_id)!=descq->send_runahead_map.end()) {
        descq->send_runahead_map[desc_id] = core->cycles - descq->send_runahead_map[desc_id];
      }
      else {
        descq->send_runahead_map[desc_id] = core->cycles;
      }
    }
    if(type == STVAL) {
      can_exit_rob=true;
      stat.update(stval_issue_success);
      core->local_stat.update(stval_issue_success);
    }
    if (type == STADDR) {
      can_exit_rob=true;
      stat.update(staddr_issue_success);
      core->local_stat.update(staddr_issue_success);
    }
    if(type == RECV) {
      stat.update(recv_issue_success);
      core->local_stat.update(recv_issue_success);
    }
    if(type == SEND) {
      stat.update(send_issue_success);
      core->local_stat.update(send_issue_success);
    }
    if(type == LD_PROD) {
      can_exit_rob=true; //allow rob to remove this if it's the head
      descq->debug_send_set.insert(desc_id);
      //a RECV could get the forwarded value before finishNode() is called
      if(can_forward_from_lsq) {
        //do nothing, ld_prod goes to term_ld_buff lsq has fed value
        //assert(false);
        lsq_fwd_count++;
        //cout << "LSQ_FWD: " << lsq_fwd_count << endl;
      }
      else if(can_forward_from_svb) { 
        //can do decoupled stl fwding
        stl_fwd_count++;
        //cout << "SVB_FWD: " << stl_fwd_count << endl;
      }
      else { //must go to memory
        print(Access_Memory_Hierarchy, 5);
        core->access(this); //send out load to mem hier       
      }
      stat.update(ld_prod_issue_success);
      core->local_stat.update(ld_prod_issue_success);
    }
  }
  return can_issue;  
}

void DynamicNode::finishNode() {

  DESCQ* descq=core->sim->get_descq(this);
  if(type==SEND || type==LD_PROD) {
    
    descq->debug_send_set.insert(desc_id);
  }
  if(type==STVAL) {
    descq->debug_stval_set.insert(desc_id);
  }
  if(completed) {
    print("already completed ", 5);
    assert(false);
  }
  if(cfg.verbLevel >= 4) {
    cout << "Cycle: " << core->cycles << " \n"; 
    print("Completed", 5);
  }
  //these assertions test to make sure decoupling dependencies are maintained
  if(n->typeInstr==RECV && descq->debug_send_set.find(desc_id)==descq->debug_send_set.end()) {
    cout << "Assertion about to fail for DESCID " << desc_id << endl;
    print("recv trying to jump", 5);
  }

  assert(!((n->typeInstr==RECV) && descq->debug_send_set.find(desc_id)==descq->debug_send_set.end()));         
  assert(!((n->typeInstr==STADDR) && descq->debug_stval_set.find(desc_id)==descq->debug_send_set.end()));

  if(type==STVAL) {
    descq->stval_runahead_map[desc_id]=core->cycles;    
  }
  if(type==STADDR) {
    descq->stval_runahead_map[desc_id] = core->cycles - descq->stval_runahead_map[desc_id];
  }
  
  c->completed_nodes.insert(this);

  completed = true;

  // Handle Resource limits
  if ( core->available_FUs.at(n->typeInstr) != -1 )
    core->available_FUs.at(n->typeInstr)++; 

  // Speculation
  if (core->local_cfg.mem_speculate && n->typeInstr == ST) {
    auto misspeculated = core->lsq.check_speculation(this);
    for(unsigned int i=0; i<misspeculated.size(); i++) {
      // Handle Misspeculation
      stat.update("misspeculated");
      core->local_stat.update("misspeculated");
      misspeculated.at(i)->issued = false;
      misspeculated.at(i)->completed = false;
      misspeculated.at(i)->tryActivate();
    }
  }

  //Handle Terminator Node, Launch Context
  //if cf_mode was 1, then all the contexts would already have been launched
  if (core->local_cfg.cf_mode == 0 && type == TERMINATOR) {
    core->context_to_create++;    
  }
  
  if(isMem) {      
    speculated = false;
  }
  // Since node <n> ended, update dependents: decrease each dependent's parent count & try to launch each dependent
  set<Node*>::iterator it;
  for (it = n->dependents.begin(); it != n->dependents.end(); ++it) {
    DynamicNode *dst = c->nodes.at(*it);

    //if the dependent is a store addr dependent
    if(n->store_addr_dependents.find(*it) != n->store_addr_dependents.end()) {      
      dst->addr_resolved = true;
      core->lsq.resolveAddress(dst);      
    }
    dst->pending_parents--;
    dst->tryActivate();    
  }

  // The same for external dependents: decrease parent's count & try to launch them
  for(unsigned int i=0; i<external_dependents.size(); i++) {
    DynamicNode *dst = external_dependents.at(i);
   
    if(n->store_addr_dependents.find(n) != n->store_addr_dependents.end()) {
      dst->addr_resolved = true;
      core->lsq.resolveAddress(dst);
    }
    dst->pending_external_parents--;
    assert(!dst->issued && !dst->completed);
    dst->tryActivate();    
  }
  external_dependents.clear();

  // Same for Phi dependents
  for (it = n->phi_dependents.begin(); it != n->phi_dependents.end(); ++it) {
    Node *dst = *it;
    if(c->next_bbid == dst->bbid) {
      if(Context *cc = c->getNextContext()) {
        cc->nodes.at(dst)->pending_parents--;
        assert(!cc->nodes.at(dst)->issued && !cc->nodes.at(dst)->completed);
        cc->nodes.at(dst)->tryActivate();
      }
    }
  }
}

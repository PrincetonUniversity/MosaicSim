#include "DynamicNode.h"
#include "Core.h"
#include "../memsys/Cache.h"
#define ID_POOL 1000000
bool Core::canAccess(bool isLoad) {
  return master->canAccess(isLoad);
}
void Core::communicate(DynamicNode *d) {
  master->communicate(d);
}
void Core::access(DynamicNode* d) {
  int tid = tracker_id.front();
  tracker_id.pop();
  access_tracker.insert(make_pair(tid, d));
  Transaction *t = new Transaction(tid, id, d->addr, d->type == LD);
  master->access(t);
}

void IssueWindow::insertDN(DynamicNode* d) {
  issueMap.insert(make_pair(d,curr_index));
  //d->print(" InsertedDN: ", -50);
  curr_index++;
}

bool IssueWindow::canIssue(DynamicNode* d) {
  assert(issueMap.find(d)!=issueMap.end());
  uint64_t position=issueMap.at(d);
  return issueCount<issueWidth && position>=window_start && position<=window_end;
}

void IssueWindow::process() {
  issueCount=0;
  vector<DynamicNode*> eraseVector;
  for(auto it=issueMap.begin(); it!=issueMap.end(); ++it) {
    if (it->first->completed) { 
      //(*it).first->print("completed node", -2); //luwa test
     
      window_start=it->second+1;
      window_end=(window_size+window_start)-1;
      eraseVector.push_back(it->first);
    }
    else {
      break;
    }
  }
  
  for (auto it=eraseVector.end(); it!=eraseVector.end(); ++it) {
    issueMap.erase(*it);
  }
  //cout << "Win start: " << window_start << " Win end: " << window_end << endl; //luwa test
}

void IssueWindow::issue() {
  assert(issueCount<=issueWidth);
  issueCount++;
}

void Core::accessComplete(Transaction *t) {
  int tid = t->id;
  DynamicNode *d = access_tracker.at(tid);
  access_tracker.erase(tid);
  tracker_id.push(tid);
  delete t;
  d->handleMemoryReturn();
}
void Core::initialize(int id) {
  // Initialize Resources / Limits
  this->id = id;
  lsq.size = local_cfg.lsq_size;
  for(int i=0; i<NUM_INST_TYPES; i++) {
    available_FUs.insert(make_pair(static_cast<TInstr>(i), local_cfg.num_units[i]));
  }
  
  // Initialize Control Flow mode: 0 = one_context_at_once  / 1 = all_contexts_simultaneously
  if (local_cfg.cf_mode == 0) 
    context_to_create = 1;
  else if (local_cfg.cf_mode == 1)  
    context_to_create = cf.size();
  else
    assert(false);
  // Initialize Activity counters
  for(int i=0; i<NUM_INST_TYPES; i++) {
    local_stat.registerStat(instrToStr(static_cast<TInstr>(i)),1);
    stat.registerStat(instrToStr(static_cast<TInstr>(i)),1);}   
  
  for(int i=0; i<ID_POOL; i++) {
    tracker_id.push(i);
  }
}

string Core::instrToStr(TInstr instr) {
  std::string InstrName[] = { "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "INVALID"};
  return InstrName[instr];
}

void Core::printActivity() {
  
  std::string InstrName[] =  { "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "INVALID"};
  cout << "-----------Simulator " << name << " Activity-----------\n";
  cout << "Cycles: " << cycles << endl;
  cout << "Mem_bytes_read: " << activity_mem.bytes_read << "\n";
  cout << "Mem_bytes_written: " << activity_mem.bytes_write << "\n";
  for(int i=0; i<NUM_INST_TYPES; i++)
    cout << "Intr[" << InstrName[i] << "]=" << activity_FUs.at(static_cast<TInstr>(i)) << "\n";  
}

bool Core::createContext() {
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
  if (local_cfg.max_active_contexts_BB > 0) {
    if(outstanding_contexts.find(bb) == outstanding_contexts.end()) {
      outstanding_contexts.insert(make_pair(bb, local_cfg.max_active_contexts_BB));
    }
    else if(outstanding_contexts.at(bb) == 0)
      return false;
    outstanding_contexts.at(bb)--;
  }
  Context *c = new Context(cid, this);
  context_list.push_back(c);
  live_context.push_back(c);
  c->initialize(bb, next_bbid, prev_bbid);

  return true;
}

bool Core::process() {
  window.process();
  if(cfg.verbLevel >= 0)
    cout << "[Cycle: " << cycles << "]\n";
  if(cycles % 100000 == 0 && cycles !=0) {
    curr = Clock::now();
    uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr - last).count();
    
    cout << name << " Simulation Speed: " << ((double)(local_stat.get("contexts") - last_processed_contexts)) / tdiff << " contexts per ms \n";
    last_processed_contexts = local_stat.get("contexts");
    last = curr;
    stat.set("cycles", cycles);
    local_stat.set("cycles", cycles);
    local_stat.print();
  }
  else if(cycles == 0) {
    last = Clock::now();
    last_processed_contexts = 0;
  }
  bool simulate = false;
  for(auto it = live_context.begin(); it!=live_context.end(); ++it) {
    Context *c = *it;
    c->process();
  }
 
  for(auto it = live_context.begin(); it!=live_context.end();) {
    
    Context *c = *it;
    c->complete();
    
    if(it!=live_context.begin()) {   
      assert((*it)->id > (*(it-1))->id); //making sure contexts are ordered      
    }
    
    if(c->live)
      it++;
    else
      it = live_context.erase(it);
  }
  if(live_context.size() > 0)
    simulate = true;
  int context_created = 0;

  for (int i=0; i<context_to_create; i++) {
    if (createContext()) {
      simulate = true;
      context_created++;
    }
    else
      break;
  }
  context_to_create -= context_created;   // some contexts can be left pending for later cycles
  cycles++;
  return simulate;
}

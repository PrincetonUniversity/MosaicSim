#include "DynamicNode.h"
#include "Core.h"
#include "../memsys/Cache.h"
#define ID_POOL 1000000

std::string InstrName[]={ "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "LD_PROD", "INVALID", "BS_DONE", "CORE_INTERRUPT", "CALL_BS", "BS_WAKE", "BS_VECTOR_INC"};

Core::Core(Simulator* sim, int clockspeed) : Tile(sim, clockspeed) {}

bool Core::canAccess(bool isLoad) {
  return sim->canAccess(this, isLoad);
}
bool Core::communicate(DynamicNode *d) {
  return sim->communicate(d);
}
void Core::access(DynamicNode* d) {
  int tid = tracker_id.front();
 
  tracker_id.pop();
  access_tracker.insert(make_pair(tid, d));
  
  MemTransaction *t = new MemTransaction(tid, id, id, d->addr, d->type == LD || d->type == LD_PROD);
  t->d=d;
  //assert(tid!=-1);
  cache->addTransaction(t);
  //sim->access(t);
}

void IssueWindow::insertDN(DynamicNode* d) {
  issueMap.insert(make_pair(d,curr_index));
  curr_index++;
}

bool IssueWindow::canIssue(DynamicNode* d) {
  assert(issueMap.find(d)!=issueMap.end());
  uint64_t position=issueMap.at(d);
  //cout << "window size is " << window_size << endl;
  //cout << "issue width is " << issueWidth << endl; 
  
  if(window_size==-1 && issueWidth==-1) { //infinite sizes
    return true;
  }
  if(window_size==-1) { //only issue width matters
    return issueCount<issueWidth;
  }
  if (issueWidth==-1) { //only instruction window availability matters
    return position>=window_start && position<=window_end;
  }
  /*
  if(d->core->cycles >= 1000000 && d->core->cycles %500000==0) {
    
    if(position<window_start || position>window_end)
      d->print("rob full", -10);
    //cout<<"STILL PROCESSING CONTEXTS \n";
  }
  */
  return issueCount<issueWidth && position>=window_start && position<=window_end;
}

void IssueWindow::process() {
  issueCount=0;
  vector<DynamicNode*> eraseVector;
  for(auto it=issueMap.begin(); it!=issueMap.end();) {
    if (it->first->completed || (it->first->n->typeInstr==LD_PROD && it->first->issued)) {//when Load_Produce is at head of RoB, you can remove it, even if it's been issued but not completed
 
      window_start=it->second+1;
      window_end=(window_size+window_start)-1;
      it=issueMap.erase(it); //erases and gets next element, i.e., it++
      //eraseVector.push_back(it->first);
    }
    else {
      break;
    }
  }
}

void IssueWindow::issue() {
  assert(issueWidth==-1 || issueCount<=issueWidth);
  issueCount++;
}

 //handle completed memory transactions
void Core::accessComplete(MemTransaction *t) {
  int tid = t->id;

  if(access_tracker.find(tid)!=access_tracker.end()) {
    DynamicNode *d = access_tracker.at(tid);
    access_tracker.erase(tid);
    tracker_id.push(tid);
    delete t;
    d->handleMemoryReturn();
  }
  else {
    cout << "assertion false for transaction" << tid << endl;
    assert(false);
  }
}

//handle completed transactions
bool Core::ReceiveTransaction(Transaction* t) {
  cout << "Received Response at Cycle: " << cycles << endl;
  t->complete=true;
  return true;
}

void Core::initialize(int id) {
  this->id = id;
    
  // Set up cache
  cache = new Cache(local_cfg.L1_latency, local_cfg.L1_size, local_cfg.L1_assoc, local_cfg.L1_linesize, local_cfg.cache_load_ports, local_cfg.cache_store_ports, local_cfg.ideal_cache);
  
  cache->sim = sim;
  cache->parent_cache=sim->cache;
  cache->isL1=true;
  cache->memInterface = sim->memInterface;
  
  // Initialize Resources / Limits
  lsq.size = local_cfg.lsq_size;
  window.window_size=local_cfg.window_size;
  window.issueWidth=local_cfg.issueWidth;
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

  for(uint64_t i=0; i<cf.size(); i++) {
    uint64_t bbid=cf[i];
    BasicBlock *bb = g.bbs.at(bbid);
    sim->total_instructions+=bb->inst_count;
  }
  //cout << "Total Instructions is: " << sim->total_instructions << endl;
  //assert(false);
  
}

string Core::instrToStr(TInstr instr) {  
  return InstrName[instr];
}

void Core::printActivity() {
  
  std::string InstrName[] =  { "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "LD_PROD", "INVALID",  "BS_DONE", "CORE_INTERRUPT", "CALL_BS", "BS_WAKE", "BS_VECTOR_INC"};
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
  bool simulate = cache->process();
  
  if(cfg.verbLevel >= 0)
    cout << "[Cycle: " << cycles << "]\n";
  if(cycles % 1000000 == 0 && cycles !=0) {
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
  
  //kill the accelerator tile, so digestor doesn't call it's process() anymore and can eventually end the simulation
  if(!simulate) {
    ExampleTransaction* newt1=new ExampleTransaction(0,id,1);
    newt1->type=TILE_COMPLETE;
    sim->InsertTransaction(newt1, cycles);
    ExampleTransaction* newt2=new ExampleTransaction(0,id,2);
    newt2->type=TILE_COMPLETE;
    sim->InsertTransaction(newt2, cycles);
  }
  return simulate;
}

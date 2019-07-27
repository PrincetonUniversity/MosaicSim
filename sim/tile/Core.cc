#include "DynamicNode.h"
#include "Core.h"
//#include "LoadStoreQ.h"
#include "../memsys/Cache.h"
#include <vector>

#define ID_POOL 1000000
using namespace std;

Core::Core(Simulator* sim, int clockspeed) : Tile(sim, clockspeed) {}

bool Core::canAccess(bool isLoad) {
  return sim->canAccess(this, isLoad);
}

bool Core::communicate(DynamicNode *d) {
  return sim->communicate(d);
}

void Core::access(DynamicNode* d) {
  //collect stats on load latency
  if(sim->debug_mode || sim->mem_stats_mode) {
    if(sim->load_stats_map.find(d)!=sim->load_stats_map.end()) {
      //d->print("assertion to fail", -10);
      //assert(false);
    }
    long long current_cycle=cycles;
    long long return_cycle=0;
    /*if(d->type==ST) {
      return_cycle=current_cycle;
    } */
    sim->load_stats_map[d]=make_tuple(current_cycle,return_cycle,false); //(issue cycle, return cycle)
  }
  /*
  int tid = tracker_id.front();
  tracker_id.pop();
  access_tracker.insert(make_pair(tid, d));
  */
  MemTransaction *t = new MemTransaction(1, id, id, d->addr, d->type == LD || d->type == LD_PROD);
  t->d=d;
  cache->addTransaction(t);
}

void IssueWindow::insertDN(DynamicNode* d) {
  issueMap.insert(make_pair(d,curr_index));
  d->windowNumber=curr_index;
 
  if(d->type==BARRIER)
    barrierVec.push_back(d);
  curr_index++;
}

bool IssueWindow::canIssue(DynamicNode* d) {
  //assert(issueMap.find(d)!=issueMap.end());
  
  uint64_t position=d->windowNumber;//issueMap.at(d);

  //make sure there are no older barrier instructions
  for(auto b:barrierVec) {
    if(*d<*b) { //break once you find 1st younger barrier b
      break;
    }
    if(*b<*d) { //if barrier is older, no younger instruction can issue 
      return false;
    }
  }
  
  if(window_size==-1 && issueWidth==-1) { //infinite sizes
    return true;
  }
  if(window_size==-1) { //only issue width matters
    return issueCount<issueWidth;
  }
  if (issueWidth==-1) { //only instruction window availability matters
    return position>=window_start && position<=window_end;
  }
  return issueCount<issueWidth && position>=window_start && position<=window_end;
}

void IssueWindow::process() {
  issueCount=0;
  //shift RoB
  for(auto it=issueMap.begin(); it!=issueMap.end();) {
    if (it->first->completed || it->first->can_exit_rob) {//when Load_Produce, STADDR, and STVAL are marked as can_exit_rob if there are resources available, meaning you can remove them from RoB when they are at the head
      it->first->stage=LEFT_ROB;
      window_start=it->second+1;
      window_end=(window_size+window_start)-1;
      it=issueMap.erase(it); //erases and gets next element, i.e., it++
    }
    else {
      break;
    }
  }
  cycles++;
}

void IssueWindow::issue() {
  assert(issueWidth==-1 || issueCount<=issueWidth);
  issueCount++;
}

//handle completed memory transactions
void Core::accessComplete(MemTransaction *t) {
  DynamicNode *d;
  d=t->d;
  delete t;
  d->handleMemoryReturn();

  /*
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
  */
}


//handle completed transactions
bool Core::ReceiveTransaction(Transaction* t) {
  
  t->complete=true;
  DynamicNode* d=t->d;
  /*
  cout << d->acc_args << endl;
  cout << "CPU Clocktick: " << cycles << "; Acc Return: " << d->acc_args << endl;
  cout << "Cycles taken: " << t->perf.cycles << endl;
  cout << "Bytes: " << t->perf.bytes << endl;
  cout << "Power: " << t->perf.power << endl;
  cout << "Acc DRAM Accesses: " << t->perf.bytes/sim->cache->size_of_cacheline  << endl;
  */
  cout << "Acc_Completed; Cycle: " << cycles <<"; Power: " << t->perf.power << "; Args: " << d->acc_args << endl;
  
  //register energy and dram access stats
  stat.update("dram_accesses",t->perf.bytes/sim->cache->size_of_cacheline); //each DRAM access is 1 cacheline
  //power is mW, freq in MHz --> 1e-3*1e-6
  double energy=(t->perf.power * t->perf.cycles * 1e-9)/cfg.chip_freq;
  stat.acc_energy+=energy;
  
  d->c->insertQ(d); //complete the dynamic node involved
  delete t;
  
  return true;
}

void Core::initialize(int id) {
  this->id = id;
    
  // Set up cache
  cache = new Cache(local_cfg);
  cache->core=this;
  cache->sim = sim;
  cache->parent_cache=sim->cache;
  cache->isL1=true;
  cache->memInterface = sim->memInterface;
  lsq.mem_speculate=local_cfg.mem_speculate;
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
    local_stat.registerStat(getInstrName((TInstr)i),1);
    stat.registerStat(getInstrName((TInstr)i),1);
  }   
  for(int i=0; i<ID_POOL; i++) {
    tracker_id.push(i);
  }
  //count the number of instructions here
  for(uint64_t i=0; i<cf.size(); i++) {
    uint64_t bbid=cf[i];
    BasicBlock *bb = g.bbs.at(bbid);
    sim->total_instructions+=bb->inst_count;
    //exit gracefully instead of getting killed by OS
  }
  
  cout << "Total Num Instructions: " << sim->total_instructions << endl;
  if(sim->instruction_limit>0 && sim->total_instructions>=sim->instruction_limit) {
    cout << "\n----SIMULATION TERMINATING----" << endl;
    cout << "Number of instructions is larger than the " << sim->instruction_limit << "-instruction limit. Please run the application with a smaller dataset." << endl;
    assert(false);
  }
  //assert(false);
}

 vector<string> InstrName={ "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "LD_PROD", "INVALID", "BS_DONE", "CORE_INTERRUPT", "CALL_BS", "BS_WAKE", "BS_VECTOR_INC", "BARRIER", "ACCELERATOR"};

string Core::getInstrName(TInstr instr) {  
 
  return InstrName[instr];
}

// Return boolean indicating whether or not the branch was mispredicted
// We can do this by looking at the context, core, etc from the DynamicNode and seeing if the next context has the same bbid as the current one
// Simple Always-Taken predictor. We'll guess that we remain in the same basic block (or loop). Hence, it's a misprediction when we change basic blocks
bool Core::predict_branch(DynamicNode* d) {
  int context_id=d->c->id;
  int next_context_id=context_id+1;

  int current_bbid=cf.at(context_id);
  
  int cf_size = cf.size();
  int next_bbid=-1;
  if(next_context_id < cf_size) {
    next_bbid=cf.at(next_context_id);
  }
  if(current_bbid==next_bbid) { // guess we'll remain in same basic block
    //cout << "CORRECT prediction \n";
    return true;   
  }
  else {   
    //cout << "WRONG prediction \n";
    return false;
  }
}

bool Core::createContext() {
  unsigned int cid = total_created_contexts;
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
  if(!lsq.checkSize(bb->ld_count, bb->st_count)) {
    return false;
  }
    
  // check the limit of contexts per BB
  if (local_cfg.max_active_contexts_BB > 0) {
    if(outstanding_contexts.find(bb) == outstanding_contexts.end()) {
      outstanding_contexts.insert(make_pair(bb, local_cfg.max_active_contexts_BB));
    }
    else if(outstanding_contexts.at(bb) == 0) {
      return false;
    }
    outstanding_contexts.at(bb)--;
  }
  Context *c = new Context(cid, this);
  context_list.push_back(c);
  live_context.push_back(c);
  c->initialize(bb, next_bbid, prev_bbid);
  total_created_contexts++;
  return true;
}

void Core::fastForward(uint64_t inc) {
  cycles+=inc;
  cache->cycles+=inc;
  window.cycles+=inc;
  if(id % 2 == 0) {
    sim->get_descq(this)->cycles+=inc;    
  }
}

bool Core::process() {
  //process the instruction window and RoB
  window.process();
  //process the private cache
  bool simulate = cache->process();

  //process descq if this is the 2nd tile. 2 tiles share 1 descq//  
  if(id % 2 == 0) {
    sim->get_descq(this)->process();    
  }

  windowFull=false;
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
    else {
      it = live_context.erase(it);
    }
  }

  if(live_context.size() > 0)
    simulate = true;
  
  // create all the needed new contexts (eg, whenever a terminator node is reached a new context must be created)
  int context_created = 0;
  for (int i=0; i<context_to_create; i++) {
    if (createContext()) {
      simulate = true;
      context_created++;
    }
    else
      break;
  }
  context_to_create -= context_created;   // note that some contexts could be left pending for later cycles
  cycles++;

  // Print current stats every "stat.printInterval" cycles
  if(cfg.verbLevel >= 5)
    cout << "[Cycle: " << cycles << "]\n";
  if(cycles % stat.printInterval == 0 && cycles != 0) {
    curr = Clock::now();
    uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr - last).count();
    
    cout << endl << "--- " << name << " Simulation Speed: " << ((double)(local_stat.get("contexts") - last_processed_contexts)) / tdiff << " contexts per ms \n";
    last_processed_contexts = local_stat.get("contexts");
    last = curr;
    stat.set("cycles", cycles);
    local_stat.set("cycles", cycles);
    local_stat.print(cout);

    // release system memory by deleting all processed contexts
    if(!sim->debug_mode) {
      deleteErasableContexts();
    }
  }
  else if(cycles == 0) {
    last = Clock::now();
    last_processed_contexts = 0;
  }
  return simulate;
}

void Core::deleteErasableContexts() {   
  int erasedContexts=0;
  int erasedDynamicNodes=0;
//  int count;
//  cout << "-------trying to erase contexts:------------------------------\n";
//  count=0; for(auto it=context_list.begin(); it != context_list.end(); ++it) if(*it) count++;
//  cout << "--  context_list size=" << count /*context_list.size()*/ << endl;
//  cout << "--  live_context_list size=" << live_context.size() << endl;

  int safetyWindow=1000000; 
  for(auto it=context_list.begin(); it != context_list.end(); ++it) {
    Context *c = *it;
    // safety measure: only erase contexts marked as erasable 1mill cycles apart
    if(c && c->isErasable && c->cycleMarkedAsErasable < cycles-safetyWindow ) {  
      // first, delete dynamic nodes associated to this context
      for(auto n_it=c->nodes.begin(); n_it != c->nodes.end(); ++n_it) {
        DynamicNode *d = n_it->second;
        delete d;   // delete the dynamic node
        erasedDynamicNodes++;
      }
      // now delete the context itself
      delete c;  
      erasedContexts++;
      *it = NULL;
    }
  }
//  cout << "-------erased contexts: " << erasedContexts << "   erased DNs: " << erasedDynamicNodes << "----------------------\n";
//  count=0; for(auto it=context_list.begin(); it != context_list.end(); ++it) if(*it) count++;
//  cout << "--  new context_list size=" << count /*context_list.size()*/ << endl;
}

void Core::calculateEnergyPower() {
  total_energy = 0.0;
  int tech_node = cfg.technology_node;

  // FIX THIS: for now we only calculate the energy for IN-ORDER cores
  if( local_cfg.window_size == 1 && local_cfg.issueWidth == 1) {
    
    // add Energy Per Instruction (EPI) class
    for(int i=0; i<NUM_INST_TYPES; i++) {
      total_energy += local_cfg.energy_per_instr.at(tech_node)[i] * local_stat.get(getInstrName((TInstr)i));
    }
    // NOTE1: the energy for accesing the L1 is already accounted for within LD/ST energy_per_instr[]
    // NOTE2: We assume the L2 is shared. So its energy will be accounted for at the chip level (in sim.cc)

    // calculate avg power
    avg_power = total_energy * clockspeed*1e+6 / cycles;  // clockspeed is defined in MHz

    // some debug stuff
    //cout << "-------Total core energy (w/ L1): " << total_energy << endl;
  }
}

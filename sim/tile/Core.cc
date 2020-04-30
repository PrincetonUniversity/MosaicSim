#include "Core.h"

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
    sim->load_stats_map[d]=make_tuple(current_cycle,return_cycle,2); //(issue cycle, return cycle)
  }
  /*
  int tid = tracker_id.front();
  tracker_id.pop();
  access_tracker.insert(make_pair(tid, d));
  */
  int graphNodeId = -1;
  int graphNodeDeg = -1;
  if (sim->graphNodeIdMap.find(d->addr) != sim->graphNodeIdMap.end()) {
    graphNodeId = sim->graphNodeIdMap[d->addr];

    //assert(sim->graphNodeDegMap.find(graphNodeId) != sim->graphNodeDegMap.end());
    graphNodeDeg = sim->graphNodeDegMap[graphNodeId];
  }
  TInstr i_type = d->type;
  if (i_type == ATOMIC_CAS || i_type == ATOMIC_MIN || i_type == ATOMIC_FADD || i_type == LLAMA || d->n->id == llamaNodeId) {
    graphNodeId = 0;
    graphNodeDeg = 0;
    if (sim->graphNodeIdMap.find(d->addr) == sim->graphNodeIdMap.end()) {
      sim->graphNodeIdMap[d->addr] = graphNodeId;
    }
  }
  MemTransaction *t = new MemTransaction(1, id, id, d->addr, d->type == LD || d->type == LD_PROD || d->type == LLAMA, graphNodeId, graphNodeDeg);
  t->d=d;

  if (partition_L1 == 1 && graphNodeId != -1) { // LLAMA access
    llama_cache->addTransaction(t);
  } else {
    cache->addTransaction(t);
  }
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
  bool can_issue=true;
   
  if(window_size==-1 && issueWidth==-1) { //infinite sizes
    can_issue=can_issue && true;
  }
  else if(window_size==-1) { //only issue width matters
    can_issue = can_issue && issueCount<issueWidth;
  }
  else if(issueWidth==-1) { //only instruction window availability matters
    can_issue = can_issue && position>=window_start && position<=window_end;
  }  
  else {
    can_issue = can_issue && issueCount<issueWidth && position>=window_start && position<=window_end;
  }

  if(can_issue) {
    for(auto b:barrierVec) {
      if(*d<*b) { //break once you find 1st younger barrier b
        break; //won't be a problem
      }
      if(*b < *d) { //if barrier is older, no younger instruction can issue 
        can_issue=false;
        break;
      }
    }
  }
  //make sure there no instructions older than the barrier that are uncompleted
  if(can_issue && d->type==BARRIER) {
    for(auto it=issueMap.begin(); it!=issueMap.end() && it->second <= window_end; it++) {
      DynamicNode* ed=it->first;
      if(*ed < *d && !ed->completed) {
        can_issue=false;
        break;
      }
      if(*d < *ed) {
        break;
      }
    }
  }
  return can_issue;  
}

void IssueWindow::process() {
  issueCount=0;
  //shift RoB
  /*
  if(issueMap.size()!=0 && issueMap.begin()->first->core->cycles > 325000000) {
    issueMap.begin()->first->print("head of rob", -10);
  }
  */
  for(auto it=issueMap.begin(); it!=issueMap.end();) {
    if (it->first->completed || it->first->can_exit_rob) {//when Load_Produce, STADDR, STVAL, terminal RMW are marked as can_exit_rob if there are resources available, meaning you can remove them from RoB when they are at the head
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

void IssueWindow::issue(DynamicNode* d) {
  assert(issueWidth==-1 || issueCount<issueWidth);
  if(d->type != PHI) // PHIs are not real instructions and do not waste an issue slot
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
  MemTransaction* mt = (MemTransaction*) t;
  if (mt && mt->graphNodeDeg == -1) {
    stat.update("dram_accesses",t->perf.bytes/sim->cache->size_of_cacheline); //each DRAM access is 1 cacheline
  } else {
    stat.update("llama_dram_accesses",t->perf.bytes/4);
  }
  
  //power is mW, freq in MHz --> 1e-3*1e-6
  double energy=(t->perf.power * t->perf.cycles * 1e-9)/cfg.chip_freq;
  stat.acc_energy+=energy;
  
  d->c->insertQ(d); //complete the dynamic node involved
  delete t;
  
  return true;
}

void Core::initialize(int id) {
  this->id = id;
  this->llamaNodeId = local_cfg.llama_node_id;
    
  // Set up caching strategies
  partition_L1 = local_cfg.partition_L1;
  partition_L2 = local_cfg.partition_L2;
  
  // Set up caches
  l2_cache = new Cache(local_cfg.l2_cache_latency, local_cfg.mshr_size, local_cfg.l2_cache_linesize, local_cfg.llama_cache_linesize, local_cfg.l2_cache_load_ports, local_cfg.l2_cache_store_ports, local_cfg.l2_ideal_cache, local_cfg.l2_prefetch_distance, local_cfg.l2_num_prefetched_lines, local_cfg.l2_cache_size, local_cfg.l2_cache_assoc, local_cfg.eviction_policy, local_cfg.cache_by_signature, local_cfg.use_l2, local_cfg.partition_ratio, local_cfg.perfect_llama, local_cfg.l2_cache_by_temperature, local_cfg.l2_node_degree_threshold);
  l2_cache->sim = sim;
  l2_cache->parent_cache=sim->cache;
  l2_cache->memInterface = sim->memInterface;

  cache = new Cache(local_cfg);
  cache->core=this;
  cache->sim = sim;
  if (local_cfg.use_l2) {
    cache->parent_cache=l2_cache;
  } else {
    cache->parent_cache=sim->cache;
  }
  cache->isL1=true;
  cache->memInterface = sim->memInterface;
  lsq.mem_speculate=local_cfg.mem_speculate;
  
  // Set up LLAMA cache
  llama_cache = new Cache(local_cfg.cache_latency, local_cfg.mshr_size, local_cfg.llama_cache_linesize, local_cfg.llama_cache_linesize, local_cfg.llama_cache_load_ports, local_cfg.llama_cache_store_ports, local_cfg.llama_ideal_cache, local_cfg.llama_prefetch_distance, local_cfg.llama_num_prefetched_lines, local_cfg.llama_cache_size, local_cfg.llama_cache_assoc, local_cfg.llama_eviction_policy, 0, local_cfg.use_l2, 2, 0, local_cfg.cache_by_temperature, local_cfg.node_degree_threshold);
  llama_cache->core=this;
  llama_cache->sim = sim;
  if (partition_L2 == 1) {
    llama_cache->parent_cache=sim->llama_cache;
  } else {
    llama_cache->parent_cache=sim->cache;
  }
  llama_cache->isL1=true;
  llama_cache->memInterface = sim->memInterface;
  
  // Initialize Resources / Limits
  lsq.size = local_cfg.lsq_size;
  window.window_size=local_cfg.window_size;
  window.issueWidth=local_cfg.issueWidth;
  for(int i=0; i<NUM_INST_TYPES; i++) {
    available_FUs.insert(make_pair(static_cast<TInstr>(i), local_cfg.num_units[i]));
  }

  // Initialize branch predictor
  bpred = new Bpred( (TypeBpred)local_cfg.branch_predictor, local_cfg.bht_size );
  bpred->misprediction_penalty = local_cfg.misprediction_penalty;
  bpred->gshare_global_hist_bits = local_cfg.gshare_global_hist_bits;

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
}

vector<string> InstrStr={"I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", 
                         "PHI", "SEND", "RECV", "STADDR", "STVAL", "LD_PROD", "INVALID", "BS_DONE", "CORE_INTERRUPT", "CALL_BS", "BS_WAKE", "BS_VECTOR_INC", 
                         "BARRIER", "ACCELERATOR", "ATOMIC_ADD", "ATOMIC_FADD", "ATOMIC_MIN", "ATOMIC_CAS", "TRM_ATOMIC_FADD", "TRM_ATOMIC_MIN", "TRM_ATOMIC_CAS", "LLAMA"};

string Core::getInstrName(TInstr instr) {  
  return InstrStr[instr];
}

// Return boolean indicating whether the branch has been correctly predicted
bool Core::predict_branch_and_check(DynamicNode* d) {
  uint64_t current_context_id = d->c->id;
  uint64_t next_context_id = current_context_id+1;
  int current_bbid = cf.at(current_context_id);

  bool is_cond = bb_cond_destinations.at(current_bbid).first;
  if( !is_cond ) {                            //if the the branch is "unconditional" there is nothing to predict
    stat.update("bpred_uncond_branches");     //and returns TRUE immediately (correctly predicted)
    return true;                                
  }

  bool actual_taken = false;
  if(next_context_id < cf.size()) {
    int next_bbid = cf.at(next_context_id);    
    set<int> &dest = bb_cond_destinations.at(current_bbid).second;
    //assert(dest.size()==1 || dest.size()==2);
    if(dest.size()==2) { //this LLVM branch has 2 destinations (note this is very common in LLVM !!)
      //int dest1 = *dest.begin();   // dest1 is assumed as the NOT-TAKEN path
      int dest2 = *dest.rbegin();  // dest2 is assumed as the TAKEN path
      actual_taken = (next_bbid == dest2);  // if going to dest2 -> TAKEN branch
    }
    else { 
      actual_taken = true; // if there is only ONE destination -> TAKEN branch
    }
  }
  else  // this is the very last branch of the program (a RET in llvm) -> TAKEN branch
    actual_taken = true;

  // check the prediction
  bool is_pred_ok = bpred->predict_and_check(/*PC*/current_bbid, actual_taken);

  // update bpred stats
  stat.update("bpred_cond_branches");
  if(is_pred_ok)
    stat.update("bpred_cond_correct_preds");
  else  
    stat.update("bpred_cond_wrong_preds");
  return is_pred_ok;
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
  l2_cache->cycles+=inc;
  llama_cache->cycles+=inc;
  window.cycles+=inc;
  if(id % 2 == 0) {
    sim->get_descq(this)->cycles+=inc;    
  }
}

bool Core::process() {
  //process the instruction window and ROB
  window.process();

  //process all the private caches
  bool cache_process = cache->process();
  bool l2_cache_process = l2_cache->process();
  bool llama_cache_process = llama_cache->process();
  bool simulate = cache_process || l2_cache_process || llama_cache_process;

  //process descq if this is the 2nd tile. 2 tiles share 1 descq
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
  if(cycles > 0 && cycles % stat.printInterval == 0) {
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
  int safetyWindow=100000; 
  for(auto it=context_list.begin(); it != context_list.end(); ++it) {
    Context *c = *it;
    // safety measure: only erase contexts marked as erasable 1mill cycles apart
    if(c && c->isErasable && c->cycleMarkedAsErasable < cycles-safetyWindow ) {  
      // first, delete dynamic nodes associated to this context
      for(auto n_it=c->nodes.begin(); n_it != c->nodes.end(); ++n_it) {
        DynamicNode *d = n_it->second;
        d->core->lsq.remove(d); //delete from LSQ
        delete d;   // delete the dynamic node
      }
      // now delete the context itself
      delete c;  
      *it = NULL;
    }
  }
}

void Core::calculateEnergyPower() {
  total_energy = 0.0;
  int tech_node = cfg.technology_node;

  if(/*in order*/ ( local_cfg.window_size == 1 && local_cfg.issueWidth == 1)  || /*ASIC models*/ ( local_cfg.window_size >= 1024 && local_cfg.issueWidth >= 1024) ||  ( local_cfg.window_size < 0 && local_cfg.issueWidth < 0)) {
    
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
  // IMPROVE THIS: for OoO we better integrate McPAT models here - currently we do it "off-line"
  else { 
    //for Xeon E7-8894V4 from McPAT on 22nm, peak power is 11.8 W per core
    //intel TDP is 165W for all cores running. Divide by number of cores (24), we get ~6.875W
    //about .5 McPAT power, which is at 2x tech node
    avg_power = 6.875;
    total_energy = avg_power * cycles / (clockspeed*1e6);
  }
}

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "Core.hpp"

using namespace std;

vector<string> InstrStr={"I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST",
			 "TERMINATOR", "PHI", "SEND", "RECV", "STADDR", "STVAL", "LD_PROD", "INVALID", "BS_DONE", "CORE_INTERRUPT",
			 "CALL_BS", "BS_WAKE", "BS_VECTOR_INC", "BARRIER", "PARTIAL_BARRIER", "ACCELERATOR", "ATOMIC_ADD", "ATOMIC_FADD", "ATOMIC_MIN",
			 "ATOMIC_CAS", "TRM_ATOMIC_FADD", "TRM_ATOMIC_MIN", "TRM_ATOMIC_CAS"};



void IssueWindow::insertDN(DynamicNode* d) {
  issueMap.insert(make_pair(d,curr_index));
  d->windowNumber=curr_index;
 
  if(d->type==BARRIER || d->type==PARTIAL_BARRIER)
    barrierVec.push_back(d);
  curr_index++;
}

bool IssueWindow::canIssue(DynamicNode* d) {
  uint64_t position=d->windowNumber;
  bool can_issue=true;
   
  if(window_size == -1 && issueWidth == -1) { //infinite sizes
    can_issue = can_issue && true;
  }
  else if(window_size == -1) { //only issue width matters
    can_issue = can_issue && issueCount < issueWidth;
  }
  else if(issueWidth==-1) { //only instruction window availability matters
    can_issue = can_issue && position>=window_start && position<=window_end;
  }  
  else {
    can_issue = can_issue && issueCount<issueWidth && position>=window_start && position<=window_end;
  }

  if(can_issue) {
    for(auto b:barrierVec) {
      if(*d < *b) { //break once you find 1st younger barrier b
        break; //won't be a problem
      }
      if(*b < *d) { //if barrier is older, no younger instruction can issue 
        can_issue=false;
        break;
      }
    }
  }
  //make sure there no instructions older than the barrier that are uncompleted
  if(can_issue && (d->type==BARRIER || d->type==PARTIAL_BARRIER)) {
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
  for(auto it=issueMap.begin(); it!=issueMap.end();) {
    if (it->first->completed || it->first->can_exit_rob) {//when Load_Produce, STADDR, STVAL, terminal RMW are marked as can_exit_rob if there are resources available, meaning you can remove them from RoB when they are at the head
      it->first->stage=LEFT_ROB;
      window_start=it->second+1;
      window_end=(window_size+window_start)-1;
      it=issueMap.erase(it); //erases and gets next element, i.e., it++
    } else {
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


// ##################### CORE #####################
Core::Core(Simulator* sim, int clockspeed, bool pilot_descq) : Tile(sim, clockspeed),
							       pilot_descq(pilot_descq),
							       verbLevel(sim->cfg->verbLevel),
							       debug_mode(sim->debug_mode),
							       mem_stats_mode(sim->mem_stats_mode),
							       technology_node(sim->cfg->technology_node){
  stats = new Statistics();
}

bool Core::canAccess(bool isLoad, uint64_t addr) {
  return cache->willAcceptTransaction(isLoad, addr);
}

void Core::access(DynamicNode* d) {
  //TODO: REINTRODUCE_STATS
  //collect stats on load latency
  /* if(debug_mode || mem_stats_mode) { */
  /*   assert(sim->load_stats_map.find(d) == sim->load_stats_map.end()); */
  /*   sim->load_stats_map[d] = make_tuple((long long) cycles,(long long) 0, 2); //(issue cycle, return cycle) */
  /* } */
  MemTransaction *t = new MemTransaction(1, id, id, d->addr, d->type == LD || d->type == LD_PROD);
  t->d=d;
  cache->addTransaction(t, -1);
}

void Core::accessComplete(MemTransaction *t) {
  DynamicNode *d;
  d = t->d;
  delete t;
  d->handleMemoryReturn();
}

void Core::initialize(int id, Simulator *sim, PP_static_Buff<pair<int, string>> *acc_comm) {
  this->id = id;
  this->sim = sim;
  this->acc_comm = acc_comm;
  
  // Set up caches
  l2_cache = new L2Cache(local_cfg, stats);
  l2_cache->parent_cache=sim->cache;

  cache = new L1Cache(local_cfg, stats);
  cache->core=this;
  cache->parent_cache=l2_cache;
  lsq.mem_speculate=local_cfg.mem_speculate;
  l2_cache->child_cache = cache;

  // Initialize Resources / Limits
  lsq.size = local_cfg.lsq_size;
  window.window_size=local_cfg.window_size;
  window.issueWidth=local_cfg.issueWidth;
  for(int i=0; i<NUM_INST_TYPES; i++)
    available_FUs.insert(make_pair(static_cast<TInstr>(i), local_cfg.num_units[i]));

  // Initialize branch predictor
  bpred = new Bpred( (TypeBpred)local_cfg.branch_predictor, local_cfg.bht_size );
  bpred->misprediction_penalty = local_cfg.misprediction_penalty;
  bpred->gshare_global_hist_bits = local_cfg.gshare_global_hist_bits;
  
  context_to_create = 1;
  
  // Initialize Activity counters
  for(int i=0; i<NUM_INST_TYPES; i++) 
    stats->registerStat(getInstrName((TInstr)i),1);
    
}

string Core::getInstrName(TInstr instr) {  
  return InstrStr[instr];
}

bool Core::predict_branch_and_check(DynamicNode* d) {
  uint64_t current_context_id = d->c->id - deleted_cf;
  uint64_t next_context_id = current_context_id+1;
  int current_bbid = cf.at(current_context_id);
  bool actual_taken = false;

  bool is_cond = bb_cond_destinations.at(current_bbid).first;
  if( !is_cond ) {                            //if the the branch is "unconditional" there is nothing to predict
    stats->update("bpred_uncond_branches");     //and returns TRUE immediately (correctly predicted)
    return true;                                
  }
  if(next_context_id < cf.size()) {
    int next_bbid = cf.at(next_context_id);    
    set<int> &dest = bb_cond_destinations.at(current_bbid).second;
    if(dest.size()==2) { //this LLVM branch has 2 destinations (note this is very common in LLVM !!)
      int dest2 = *dest.rbegin();  // dest2 is assumed as the TAKEN path
      actual_taken = (next_bbid == dest2);  // if going to dest2 -> TAKEN branch
    } else { 
      actual_taken = true; // if there is only ONE destination -> TAKEN branch
    }
  } else  // this is the very last branch of the program (a RET in llvm) -> TAKEN branch
    actual_taken = true;

  // check the prediction
  bool is_pred_ok = bpred->predict_and_check(/*PC*/current_bbid, actual_taken);

  // update bpred stats
  stats->update("bpred_cond_branches");
  if(is_pred_ok)
    stats->update("bpred_cond_correct_preds");
  else  
    stats->update("bpred_cond_wrong_preds");
  return is_pred_ok;
}

void
Core::read_dyn_data(int *data) {
  int nb_mem, nb_cf, cf_start, *new_cf, *new_mem;
  
  if( finished_dyn_data)
    assert(false);
  
  cf_start = cf.size();
  
  nb_cf   = data[1];
  nb_mem  = data[2]; 
  new_cf  = &data[3];
  new_mem = &data[1024];
  assert( nb_cf || nb_mem);
  
  for(int i = 0; i < nb_cf; i++) {
    int new_bbid;
    set<int> destinations;
    if (new_cf[0] == -1) {
      assert( i+1 == nb_cf);
      cout <<"[SIM] ...Finished reading the Control-Flow trace for core " << id << " ! - Total contexts: " << cf.size() + deleted_cf << "\n\n";
      finished_dyn_data = true;
      break;
    }

    new_bbid = new_cf[1]; 
    
    if(new_bbid != last_bbid && last_bbid != -1) {
      cout << "[WARNING] non-continuous control flow path \n";
    }
    cf.push_back(new_bbid);
    bb_cond_destinations.insert( make_pair(new_bbid, make_pair(false, destinations)) );
    if(new_cf[0] == 1) 
      bb_cond_destinations.at(new_bbid).first=true;   // mark it is a conditional branch
    last_bbid = new_cf[2];
    new_cf += 3;
  }
  if (finished_dyn_data) {
    set<int> dest;
    cf.push_back(last_bbid);
    bb_cond_destinations.insert( make_pair(last_bbid, make_pair(false, dest)) );
  }

  /* for each conditional branch, annotate its destinations: {dest1,dest2,...} */
  for(int64_t i = cf_start; i < (int64_t) (cf.size()-1); i++) {
    if(bb_cond_destinations.at(cf[i]).first) {  // check if it is a cond branch
      bb_cond_destinations.at(cf[i]).second.insert(cf[i+1]); // insert its "next_bbid" as a destination
    }
  }

  for(int i = 0; i < nb_mem; i++) {
    new_mem -= 5;
    if (*new_mem == -1) {
      cout << "[SIM] ...Finished reading the Memory trace for core " << id << " ! Read " << memory.size() << " different elemets!" << endl;
      assert(finished_dyn_data);
      assert(i+1 == nb_mem);
      return;
    }
    int id =  new_mem[1];
    uint64_t *adress = (uint64_t *) &new_mem[2];
    if(memory.find(id) == memory.end()) 
      memory.insert(make_pair(id, queue<uint64_t>()));
    memory.at(id).push(*adress);  // insert the <address> into the memory instructions's <queue>
  }
}

bool
Core::createContext() {
  uint64_t cid = total_created_contexts;
  uint64_t cf_cid = cid - deleted_cf;
  int bbid;
  int next_bbid, prev_bbid;
  BasicBlock *bb;
  Context *c;
  
  // reached end of <cf> and we readall of them
  if(finished_dyn_data && (cf.size() + deleted_cf) == cid)
      return false;

  bbid = cf.at(cf_cid);

  // set "current", "prev", "next" BB ids.
  if ((cf.size() + deleted_cf) == cid+1 && finished_dyn_data)
    next_bbid = -1;
  else
    next_bbid = cf.at(cf_cid+1);
  if (cid != 0)
    prev_bbid = cf.at(cf_cid-1);
  else
    prev_bbid = -1;
  
  bb = g.bbs.at(bbid);
  // Check LSQ Availability
  if(!lsq.checkSize(bb->ld_count, bb->st_count))
    return false;
    
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

  // Creates the new context and adds it to cotnex_list and live_context
  c = new Context(cid, this, verbLevel);
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
  window.cycles+=inc;
  
  if(pilot_descq) {
    descq->cycles+=inc;
  }
}

bool Core::process() {
  //process the instruction window and ROB
  window.process();

  //process all the private caches
  bool cache_process = cache->process();
  bool l2_cache_process = l2_cache->process();
  bool simulate = cache_process || l2_cache_process;

  // Resize cf if needed
  if (cf.size() > max_cf_size && total_created_contexts > deleted_cf + cf.size() ) {
    cf.erase(cf.begin(), cf.begin()+100000);
    cf.shrink_to_fit();
    deleted_cf += 100000;
  }
  
  //process descq if this is the 2nd tile. 2 tiles share 1 descq
  if(pilot_descq) {
    descq->process();
  }

  windowFull=false;
  // Process all live contexts 
  for(auto it = live_context.begin(); it!=live_context.end(); ++it) {
    Context *c = *it;
    c->process();
  }
 
  // try to complete the contexts 
  for(auto it = live_context.begin(); it!=live_context.end();) {
    Context *c = *it;
    c->complete();
    
    if(it != live_context.begin()) {   
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
    } else
      break;
  }

  context_to_create -= context_created;   // note that some contexts could be left pending for later cycles
  cycles++;
  
  // Print current stats every "stat.printInterval" cycles
  if(verbLevel >= 5)
    cout << "[Cycle: " << cycles << "]\n";

 if(cycles == 0) {
    last = Clock::now();
    last_processed_contexts = 0;
 } else { 
   if(verbLevel > 0 && cycles % stats->printInterval == 0) {
     curr = Clock::now();
     uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr - last).count();
     
     cout << endl << "--- " << name << " Simulation Speed: " << ((double)(stats->get("contexts") - last_processed_contexts)) / tdiff << " contexts per ms \n";
     last_processed_contexts = stats->get("contexts");
     last = curr;
     stats->set("cycles", cycles);
     stats->print(cout);
   }
   if(cycles % 50000 == 0) 
     // release system memory by deleting all processed contexts
     deleteErasableContexts();
 }

 return simulate;
}

void Core::deleteErasableContexts() {   
  uint64_t safetyWindow = cycles > 1000000 ? 1000000 : cycles;
  for(auto it=context_list.begin(); it != context_list.end(); ) {
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
      it = context_list.erase(it);
      total_destroied_contexts++;
    } else {
      ++it;
    }
  }
}

void Core::calculateEnergyPower() {
  total_energy = 0.0;
  if(/*in order*/ ( local_cfg.window_size == 1 && local_cfg.issueWidth == 1)  || /*ASIC models*/ ( local_cfg.window_size >= 1024 && local_cfg.issueWidth >= 1024) ||  ( local_cfg.window_size < 0 && local_cfg.issueWidth < 0)) {
    
    // add Energy Per Instruction (EPI) class
    for(int i=0; i<NUM_INST_TYPES; i++) {
      total_energy += local_cfg.energy_per_instr.at(technology_node)[i] * stats->get(getInstrName((TInstr)i));
    }
    // NOTE1: the energy for accesing the L1 is already accounted for within LD/ST energy_per_instr[]
    // NOTE2: We assume the L2 is shared. So its energy will be accounted for at the chip level (in sim.cc)

    // calculate avg power
    avg_power = total_energy * clockspeed*1e+6 / cycles;  // clockspeed is defined in MHz
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


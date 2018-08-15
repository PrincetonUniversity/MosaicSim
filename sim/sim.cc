#include "header.h"
using namespace std;

Simulator::Simulator() {}
Simulator::Simulator(string name):name(name) {}

void Simulator::toMemHierarchy(DynamicNode* d) {
  cache->addTransaction(d);
}

void Simulator::initialize() {
  // Initialize Resources / Limits

  cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->memInterface = memInterface;  
    
  lsq.size = cfg.lsq_size;
  for(int i=0; i<NUM_INST_TYPES; i++) {
    available_FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
  }
  
  // Initialize Control Flow mode: 0 = one_context_at_once  / 1 = all_contexts_simultaneously
  if (cfg.cf_mode == 0) 
    context_to_create = 1;
  else if (cfg.cf_mode == 1)  
    context_to_create = cf.size();
  else
    assert(false);

  // Initialize Activity counters
  for(int i=0; i<NUM_INST_TYPES; i++) {
    activity_FUs.insert(make_pair(static_cast<TInstr>(i), 0));
  }
  activity_mem.bytes_read = 0;
  activity_mem.bytes_write = 0;
}

void Simulator::printActivity() {
  std::string InstrName[] = { "I_ADDSUB", "I_MULT", "I_DIV", "I_REM", "FP_ADDSUB", 
  "FP_MULT", "FP_DIV", "FP_REM", "LOGICAL", "CAST", "GEP", "LD", "ST", "TERMINATOR", "PHI"};

  cout << "------Simulator " << name << " ---Activity-----------------------\n";
  cout << "Mem_bytes_read: " << activity_mem.bytes_read << "\n";
  cout << "Mem_bytes_written: " << activity_mem.bytes_write << "\n";
  for(int i=0; i<NUM_INST_TYPES; i++)
    cout << "Intr[" << InstrName[i] << "]=" << activity_FUs.at(static_cast<TInstr>(i)) << "\n";
}

bool Simulator::createContext() {
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
  if (cfg.max_active_contexts_BB > 0) {
    if(outstanding_contexts.find(bb) == outstanding_contexts.end()) {
      outstanding_contexts.insert(make_pair(bb, cfg.max_active_contexts_BB));
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

void Simulator::process_memory() {
  memInterface->mem->update();
}

bool Simulator::process_cycle() {
  if(cfg.verbLevel >= 0)
    cout << "[Cycle: " << cycles << "]\n";
  if(cycles % 100000 == 0 && cycles !=0) {
    curr = Clock::now();
    uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr - last).count();
    cout << "Simulation Speed: " << ((double)(stat.get("contexts") - last_processed_contexts)) / tdiff << " contexts per ms \n";
    last_processed_contexts = stat.get("contexts");
    last = curr;
    stat.set("cycles", cycles);
    stat.print();
  }
  else if(cycles == 0) {
    last = Clock::now();
    last_processed_contexts = 0;
  }
  bool simulate = false;
  ports[0] = cfg.cache_load_ports;
  ports[1] = cfg.cache_store_ports;
  ports[2] = cfg.mem_load_ports;
  ports[3] = cfg.mem_store_ports;
  for(auto it = live_context.begin(); it!=live_context.end(); ++it) {
    Context *c = *it;
    c->process();
  }
  for(auto it = live_context.begin(); it!=live_context.end();) {
    Context *c = *it;
    c->complete();
    if(c->live)
      it++;
    else
      it = live_context.erase(it);
  }
  if(live_context.size() > 0)
    simulate = true;
  int context_created = 0;
  for (int i=0; i<context_to_create; i++) {
    if ( createContext() ) {
      simulate = true;
      context_created++;
    }
    else
      break;
  }
  context_to_create -= context_created;   // some contexts can be left pending for later cycles
  if(cache->process_cache())
    simulate = true;
  process_memory();

  cycles++;
  cache->cycles++;
  return simulate;
}

void Simulator::run() {
  bool simulate = true;
  while (simulate)
    simulate = process_cycle();
  stat.set("cycles", cycles);
  stat.print();
  memInterface->mem->printStats(true);
  printActivity();
}

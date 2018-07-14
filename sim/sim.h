#include "memory.cc"
using namespace std;

class Simulator 
{
public:
  Graph g;
  Config cfg;
  uint64_t cycles = 0;
  DRAMSimInterface* cb; 
  Cache* cache;
  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;

  int context_to_create = 0;

  /* Resources / limits */
  map<TInstr, int> available_FUs;
  map<BasicBlock*, int> outstanding_contexts;
  int ports[2]; // ports[0] = loads; ports[1] = stores;
  
  /* Profiled */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /* LSQ */
  LoadStoreQ lsq;

  void toMemHierarchy(DynamicNode* d) {
    cache->addTransaction(d);
  }
 
  void initialize() {
    // Initialize Resources / Limits
    cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.ideal_cache);
    cb = new DRAMSimInterface(cache, cfg.ideal_cache);
    cache->memInterface = cb;
    lsq.size = cfg.lsq_size;
    for(int i=0; i<NUM_INST_TYPES; i++) {
      available_FUs.insert(make_pair(static_cast<TInstr>(i), cfg.num_units[i]));
    }
    if (cfg.cf_mode == 0) 
      context_to_create = 1;
    else if (cfg.cf_mode == 1)  
      context_to_create = cf.size();
    else
      assert(false);
  }

  bool createContext() {
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
    c->initialize(bb, &cfg, next_bbid, prev_bbid);
    return true;
  }

  void process_memory() {
    cb->mem->update();
  }

  bool process_cycle() {
    if(cfg.vInputLevel > 0)
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
    ports[0] = cfg.load_ports;
    ports[1] = cfg.store_ports;
    lsq.process();
    for(auto it = live_context.begin(); it!=live_context.end(); ++it) {
      Context *c = *it;
      c->process();
    }
    lsq.clear();
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
    cache->process_cache();
    cycles++;
    cache->cycles++;
    process_memory();
    return simulate;
  }

  void run() {
    bool simulate = true;
    while (simulate)
      simulate = process_cycle();
    stat.set("cycles", cycles);
    stat.print();
    cb->mem->printStats(true);
  }
};

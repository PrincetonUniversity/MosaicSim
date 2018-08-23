#ifndef SIM_H
#define SIM_H
#include <unordered_map>
#include <queue>
#include <chrono>
#include "common.h"
#include "executionModel/DynamicNode.h"
#include "graph/Graph.h"
#include "memsys/LoadStoreQ.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "base.h"

using namespace std;

class Interconnect;
class Digestor;
class Context;

extern Config cfg;
extern Statistics stat;

typedef chrono::high_resolution_clock Clock;

class Simulator 
{
public:
  int issue_count=0; //luwa delete
  string name; 
  Graph g;
  uint64_t cycles = 0;
  DRAMSimInterface* memInterface;
  Interconnect* intercon;
  Cache* cache;
  Digestor* digestor;
  Statistics local_stat;
  bool has_digestor=false;
  chrono::high_resolution_clock::time_point curr;
  chrono::high_resolution_clock::time_point last;
  uint64_t last_processed_contexts;

  vector<Context*> context_list;
  vector<Context*> live_context;
  int context_to_create = 0;

  /* Resources / limits */
  map<TInstr, int> available_FUs;
  map<BasicBlock*, int> outstanding_contexts;
    
  /* Activity counters */
  map<TInstr, int> activity_FUs;
  struct {
    int bytes_read;
    int bytes_write;
  } activity_mem;

  /* Profiled */
  vector<int> cf; // List of basic blocks in "sequential" program order 
  unordered_map<int, queue<uint64_t> > memory; // List of memory accesses per instruction in a program order
  
  /* Handling External/Phi Dependencies */
  unordered_map<int, Context*> curr_owner;
  
  /* LSQ */
  LoadStoreQ lsq;
  void initialize();
  bool createContext();
  bool process_cycle();
  void process_memory();
  void run();
  void toMemHierarchy(DynamicNode *d);
  void printActivity();
};

class Interconnect {
public:
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  uint64_t cycles=0;
  int latency=2;
  
  void process() {
    while(pq.size()>0) {
      if(pq.top().second > cycles) {
        break;
      }      
      execute(pq.top().first);
    }
    cycles++;
  }

  void execute(DynamicNode* d) {
    d->c->insertQ(d);
  }

  void insert(DynamicNode* d) {
    pq.push(make_pair(d,cycles+latency));    
  }
};

class Digestor {
public:  
  vector<Simulator*> all_sims;
  Interconnect* intercon;
  Cache* cache;
  DRAMSimInterface* memInterface;

  
  Digestor() {        
    intercon = new Interconnect();
    cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
    memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
    cache->memInterface = memInterface;
    cache->ports[0] = cfg.cache_load_ports;
    cache->ports[1] = cfg.cache_store_ports;
    cache->ports[2] = cfg.mem_load_ports;
    cache->ports[3] = cfg.mem_store_ports;
  }

  void run() {
    bool simulate=true;
    vector<Simulator*> live_sims=all_sims;
    while(simulate) {
      vector<Simulator*> next_sims;
      simulate=false;
      for (auto it=live_sims.begin(); it!=live_sims.end(); ++it) {
        Simulator* sim=*it;
        cout << sim->name << endl;
        if(sim->process_cycle()) {
          next_sims.push_back(sim);
          simulate=true;
        }
        else {        
          stat.set("cycles", sim->cycles);
          sim->local_stat.set("cycles", sim->cycles);
        }
      }
      if(cache->process_cache())
        simulate = true;
      memInterface->mem->update();
      intercon->cycles++;
      live_sims=next_sims;
      next_sims.clear();
    }
    
    for (auto it=all_sims.begin(); it!=all_sims.end(); it++) {
      Simulator* sim=*it;
      cout << "----------------" << sim->name << " LOCAL STATS--------------\n";
      sim->printActivity();
      cout << "----------------" << sim->name << " General Stats--------------\n";
      sim->local_stat.print();      
    }
    
    cout << "----------------GLOBAL STATS--------------\n";
    stat.print();
    //cout << "-------------MEM DATA---------------\n";
    memInterface->mem->printStats(true);
  }  
};

#endif
#include "sim.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "core/Core.h"
using namespace std;

void Interconnect::process() {
  while(pq.size()>0) {
    if(pq.top().second > cycles) {
      break;
    }      
    execute(pq.top().first);
    pq.pop();
  }
  cycles++;
}

void Interconnect::execute(DynamicNode* d) {
  vector<Core*> sims=d->sim->digestor->all_sims;
  for (auto it=sims.begin(); it!=sims.end(); ++it) {
    Core* sim=*it;
    if (!(sim->name.compare("Compute"))) {
      sim->inputQ.push(d);      
    }    
  }
}

void Interconnect::insert(DynamicNode* d) {
  pq.push(make_pair(d,cycles+latency));    
}


Digestor::Digestor() {        
  intercon = new Interconnect();
  cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->memInterface = memInterface;
  cache->ports[0] = cfg.cache_load_ports;
  cache->ports[1] = cfg.cache_store_ports;
  cache->ports[2] = cfg.mem_load_ports;
  cache->ports[3] = cfg.mem_store_ports;
}

void Digestor::run() {
  bool simulate=true;
  vector<Core*> live_sims=all_sims;
  while(simulate) {
    vector<Core*> next_sims;
    simulate=false;
    for (auto it=live_sims.begin(); it!=live_sims.end(); ++it) {
      Core* sim=*it;
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
    intercon->process();
    live_sims=next_sims;
    next_sims.clear();
  }
  
  for (auto it=all_sims.begin(); it!=all_sims.end(); it++) {
    Core* sim=*it;
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
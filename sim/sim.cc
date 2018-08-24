#include "sim.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "core/Core.h"
#include "misc/Reader.h"
#include "graph/GraphOpt.h"
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
  vector<Core*> sims=d->sim->simulator->cores;
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


Simulator::Simulator() {        
  intercon = new Interconnect();
  cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->memInterface = memInterface;
  cache->ports[0] = cfg.cache_load_ports;
  cache->ports[1] = cfg.cache_store_ports;
  cache->ports[2] = cfg.mem_load_ports;
  cache->ports[3] = cfg.mem_store_ports;
}

void Simulator::run() {
  int res = 0;
  while(res > 0) {
    res = 0;
    for (auto it=cores.begin(); it!=cores.end(); ++it) {
      Core* sim = *it;
      res += sim->process();
    }
    res += cache->process();
    memInterface->process();
    intercon->process();
  }
  for (auto it=cores.begin(); it!=cores.end(); it++) {
    Core* sim=*it;
    stat.set("cycles", sim->cycles);
    sim->local_stat.set("cycles", sim->cycles);
    cout << "----------------" << sim->name << " LOCAL STATS--------------\n";
    sim->printActivity();
    cout << "----------------" << sim->name << " General Stats--------------\n";
    sim->local_stat.print();      
  }
  
  cout << "----------------GLOBAL STATS--------------\n";
  stat.print();
  memInterface->mem->printStats(true);
}
void Simulator::registerCore(string wlpath) {
  string name = "Pythia Core";
  // if (i%2==0)
  //   name="Supply";
  // else
  //   name="Compute";
  string cname = wlpath + "/output/ctrl.txt";     
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";   
  
  Core* sim = new Core();
  sim->name=name;
  Reader r;
  r.readGraph(gname, sim->g);
  r.readProfMemory(mname , sim->memory);
  r.readProfCF(cname, sim->cf);
  
  GraphOpt opt(sim->g);
  opt.inductionOptimization();
  
  sim->initialize();
  sim->simulator=this;
  sim->has_simulator=true;
  sim->cache = cache;
  sim->memInterface = memInterface;
  sim->intercon = intercon;    
  cores.push_back(sim);
  
}
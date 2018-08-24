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
  /*vector<Core*> cores=d->core->master->cores;
  for (auto it=cores.begin(); it!=cores.end(); ++it) {
    Core* core=*it;
    if (!(core->name.compare("Compute"))) {
      core->inputQ.push(d);      
    }    
  }*/
}

void Interconnect::insert(DynamicNode* d) {
  pq.push(make_pair(d,cycles+latency));    
}


Simulator::Simulator() {        
  intercon = new Interconnect();
  cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.cache_load_ports, cfg.cache_store_ports, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->sim = this;
  cache->memInterface = memInterface;
}
void Simulator::access(Transaction *t) {
  cache->addTransaction(t);
}
void Simulator::accessComplete(Transaction *t) {
  Core *core = cores.at(t->coreId);
  core->accessComplete(t);
} 
void Simulator::run() {
  int res = 0;
  while(res > 0) {
    res = 0;
    for (auto it=cores.begin(); it!=cores.end(); ++it) {
      Core* core = *it;
      res += core->process();
    }
    res += cache->process();
    memInterface->process();
    intercon->process();
  }
  for (auto it=cores.begin(); it!=cores.end(); it++) {
    Core* core=*it;
    stat.set("cycles", core->cycles);
    core->local_stat.set("cycles", core->cycles);
    cout << "----------------" << core->name << " LOCAL STATS--------------\n";
    core->printActivity();
    cout << "----------------" << core->name << " General Stats--------------\n";
    core->local_stat.print();      
  }
  
  cout << "----------------GLOBAL STATS--------------\n";
  stat.print();
  memInterface->mem->printStats(true);
}
void Simulator::registerCore(string wlpath, int id) {
  string name = "Pythia Core";
  // if (i%2==0)
  //   name="Supply";
  // else
  //   name="Compute";
  string cname = wlpath + "/output/ctrl.txt";     
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";   
  
  Core* core = new Core();
  core->name=name;
  Reader r;
  r.readGraph(gname, core->g);
  r.readProfMemory(mname , core->memory);
  r.readProfCF(cname, core->cf);
  
  GraphOpt opt(core->g);
  opt.inductionOptimization();
  
  core->initialize(id);
  core->master=this;
  core->cache = cache;
  core->memInterface = memInterface;
  core->intercon = intercon;    
  cores.push_back(core);
  
}
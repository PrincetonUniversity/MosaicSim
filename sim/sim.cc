#include "sim.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "core/Core.h"
#include "misc/Reader.h"
#include "graph/GraphOpt.h"
#include "core/Core.h"
using namespace std;

void DESCQ::process() {  
  while(true) {        
    if(!supply_q.empty() && supply_q.front().second <= cycles) {
      execute(supply_q.front().first);
      supply_q.pop_front();
      supply_count++;
    }
    else
      break;
  }

  while(true) {
    if (supply_count==0) {
      break;
    }
    if(!consume_q.empty() && consume_q.front().second <= cycles) {
      execute(consume_q.front().first);
      consume_q.pop_front();
      supply_count--;
    }
    else
      break;
  }
  cycles++;
}

void DESCQ::execute(DynamicNode* d) {
  d->c->insertQ(d);
}

void DESCQ::insert(DynamicNode* d) {
  if(d->n->typeInstr==SEND)
    supply_q.push_back(make_pair(d, cycles+latency));
  else if(d->n->typeInstr==RECV) 
    consume_q.push_back(make_pair(d, cycles+latency)); 
}

Simulator::Simulator() {        
  descq = new DESCQ();
  cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.cache_load_ports, cfg.cache_store_ports, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->sim = this;
  cache->memInterface = memInterface;
}

bool Simulator::canAccess(bool isLoad) {
  if(isLoad)
    return cache->free_load_ports > 0;
  else
    return cache->free_store_ports > 0;
}

void Simulator::issueDESC(DynamicNode *d) {
  descq->insert(d);
}

void Simulator::access(Transaction *t) {
  cache->addTransaction(t);
}
void Simulator::accessComplete(Transaction *t) {
  Core *core = cores.at(t->coreId);
  core->accessComplete(t);
} 
void Simulator::run() {
  int simulate = 1;
  while(simulate > 0) {
    simulate = 0;
    for (auto it=cores.begin(); it!=cores.end(); ++it) {
      Core* core = *it;
      simulate += core->process();
    }
    simulate += cache->process();
    memInterface->process();
    descq->process();
  }
  for (auto it=cores.begin(); it!=cores.end(); it++) {
    Core* core=*it;
    stat.set("cycles", core->cycles);
    core->local_stat.set("cycles", core->cycles);
    cout << "----------------" << core->name << " General Stats--------------\n";
    core->local_stat.print();      
  }
  
  cout << "----------------GLOBAL STATS--------------\n";
  stat.print();
  memInterface->mem->printStats(true);
}
void Simulator::registerCore(string wlpath, string cfgname, int id) {
  string name = "Pythia Core";
  string cfgpath = "../sim/config/" + cfgname+".txt";
  string cname = wlpath + "/output/ctrl.txt";     
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";   
  
  Core* core = new Core();
  core->local_cfg.read(cfgpath);
  core->name=name;
  Reader r;
  r.readGraph(gname, core->g);
  r.readProfMemory(mname , core->memory);
  r.readProfCF(cname, core->cf);
  
  //GraphOpt opt(core->g);
  //opt.inductionOptimization();
  
  core->initialize(id);
  core->master=this;
  cores.push_back(core);  
}

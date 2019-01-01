#include "sim.h"
#include "memsys/Cache.h"
#include "memsys/DRAM.h"
#include "core/Core.h"
#include "misc/Reader.h"
#include "graph/GraphOpt.h"
#include "core/Core.h"
using namespace std;

void DESCQ::process() {
  vector<DynamicNode*> failed_nodes;
  while(true) {
    if (pq.empty() || pq.top().second >= cycles)
      break;   
    if(!execute(pq.top().first)) {
      failed_nodes.push_back(pq.top().first);
    }
    pq.pop();    
  }
  for(auto& d:failed_nodes) {
    pq.push(make_pair(d,cycles));
  }
  cycles++;
}

bool DESCQ::execute(DynamicNode* d) {
  //int predecessor_send=d->desc_id-1; 
  if (d->n->typeInstr==SEND) { //sends can commit out of order
    d->c->insertQ(d);
    debug_send_set.insert(d->desc_id);
    supply_count--;
    return true;    
  }
  else if (d->n->typeInstr==STVAL) { //sends can commit out of order
    d->c->insertQ(d);
    debug_stval_set.insert(d->desc_id); 
    return true;    
  }
  else if (d->n->typeInstr==RECV) { //make sure recvs complete after corresponding send
    if(send_map.find(d->desc_id)==send_map.end()) {
      return false;
    }
    else if (send_map.at(d->desc_id)->completed) {
      d->c->insertQ(d);
      consume_count--;
      return true;      
    }
  }

  else if (d->n->typeInstr==STADDR) { //make sure staddrs complete after corresponding send
    if(stval_map.find(d->desc_id)==stval_map.end()) {
      return false;
    }
    else if (stval_map.at(d->desc_id)->completed) {
      d->c->insertQ(d);
      d->print("Access Memory Hierarchy", 1);
      d->core->access(d);
      return true;      
    }
  }
  return false;
}

bool DESCQ::insert(DynamicNode* d) {
  if (d->n->typeInstr==SEND) {    
    if (supply_count==supply_size)  
      return false;
    else
      supply_count++;  
  }
  if (d->n->typeInstr==RECV) {
    if (consume_count==consume_size) 
      return false;
    else
      consume_count++;
  }    
  pq.push(make_pair(d,cycles+latency));
  return true;
}

Simulator::Simulator() {        
  descq = new DESCQ();
  //cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.cache_load_ports, cfg.cache_store_ports, cfg.ideal_cache);
  memInterface = new DRAMSimInterface(this, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  //cache->sim = this;
  //cache->memInterface = memInterface;
}

bool Simulator::canAccess(Core* core, bool isLoad) {
  Cache* cache = core->cache;
  if(isLoad)
    return cache->free_load_ports > 0 || cache->load_ports==-1;
  else
    return cache->free_store_ports > 0 || cache->store_ports==-1;
}

bool Simulator::communicate(DynamicNode* d) {
  return descq->insert(d);
}

void Simulator::orderDESC(DynamicNode* d) {
  if(d->n->typeInstr == SEND) {     
      descq->send_map.insert(make_pair(descq->last_send_id,d));
      d->desc_id=descq->last_send_id;
      descq->last_send_id++;
  }
  else if(d->n->typeInstr == STVAL) {
      descq->stval_map.insert(make_pair(descq->last_stval_id,d));
      d->desc_id=descq->last_stval_id;
      descq->last_stval_id++;
  }
  else if (d->n->typeInstr == RECV) {
     d->desc_id=descq->last_recv_id;
     descq->last_recv_id++;
  }
  else if (d->n->typeInstr == STADDR) {
    d->desc_id=descq->last_staddr_id;
    descq->last_staddr_id++;
  }
}

void Simulator::InsertCaches(const vector<Transaction*>& transVec) {
  for (auto t:transVec) {
    Core *core = cores.at(t->coreId);
    Cache* c = core->cache;
    int64_t evictedAddr = -1;
    uint64_t addr = t->addr;
    
    if(!(c->ideal))
      c->fc->insert(addr/64, &evictedAddr);
    if(evictedAddr!=-1) { //if ideal evicted Addr still is -1
      assert(evictedAddr >= 0);
      stat.update("cache_evict");
      c->to_evict.push_back(evictedAddr*64);
    }
  }
}

void Simulator::TransactionComplete(Transaction* t) {
  Core *core = cores.at(t->coreId);
  Cache* cache = core->cache;
  cache->TransactionComplete(t);
}

void Simulator::access(Transaction *t) {
  Core *core = cores.at(t->coreId);
  Cache* cache = core->cache;
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
    //simulate += cache->process();
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
  core->master=this;
  core->initialize(id);
  cores.push_back(core);  
}

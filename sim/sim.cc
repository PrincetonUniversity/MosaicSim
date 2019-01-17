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
  
  //drain the terminal load buffer (possibly) up to the max size
  int term_buffer_count=0; 
  while(term_buffer_count<term_buffer_size && term_ld_buffer.size()>0) {
    DynamicNode* dn=term_ld_buffer.front();
    if (insert(dn)) {
      term_ld_buffer.pop_front();
      term_buffer_count++;
    }
    else {
      break;
    }
  }
  cycles++;
}

bool DESCQ::execute(DynamicNode* d) {
  //int predecessor_send=d->desc_id-1;
  if (d->n->typeInstr==SEND || d->n->typeInstr==LD_PROD) { //sends can commit out of order
    d->c->insertQ(d);
    debug_send_set.insert(d->desc_id);
    supply_count--;
    return true;    
  }
  else if (d->n->typeInstr==STVAL) { //sends can commit out of order
    if(!stvalFwdPending(d)) { //must wait until all fwds are sent
     
      d->c->insertQ(d);
      debug_stval_set.insert(d->desc_id);
      return true;    
    }

  }
  else if (d->n->typeInstr==RECV) {
    //make sure recvs complete after corresponding send
    //or they can get a fwd from SVB

    if (decrementFwdCounter(d)) {//can you get fwd from SVB
      d->c->insertQ(d);
      consume_count--;
      
      return true;      
    }
    
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

void DESCQ::updateSVB(DynamicNode* lpd, DynamicNode* staddr_d) {
  stval_svb_map[staddr_d->desc_id]++;
  //want to increment the counter for fwds using staddr_d's desc id (really, for stval's desc id), that way stval knows when it's safe to complete
  recv_map[lpd->desc_id]=staddr_d->desc_id;
  //also want to create a mapping from ld_prod's descid (really for recv) to stval's desc id above. that way recv can know if there's a fwd waiting for it and also decrement the stval's counter
 
}

bool DESCQ::updateSAB(DynamicNode* lpd) { //called by issuedescnode
  
  uint64_t addr=lpd->addr;
  if(staddr_map.find(addr)==staddr_map.end()) { //we've deleted the entry for corresponding staddr
    return false;
  }
  
  set<DynamicNode*, DynamicNodePointerCompare> staddr_set=staddr_map.at(addr);
  bool found_fwd=false;
  auto it = staddr_set.begin();
  while(it!=staddr_set.end()) {    
    if(*lpd < **it) {
      if(it!=staddr_set.begin()) {
        it--;
        found_fwd=true;
      }
      break;
    }
    it++;
  }
  if(found_fwd) {
    DynamicNode* staddr_d=*it;
    updateSVB(lpd, staddr_d);
  }
  return found_fwd; 
}

void DESCQ::insert_staddr_map(DynamicNode* d) {
 
  if(staddr_map.find(d->addr)==staddr_map.end()) {      
    set<DynamicNode*, DynamicNodePointerCompare> temp;
    temp.insert(d);
    staddr_map[d->addr]=temp;
  }
  else {
    staddr_map[d->addr].insert(d);
  }  
}

bool DESCQ::decrementFwdCounter(DynamicNode* recv_d) {
  
  if(recv_map.find(recv_d->desc_id)==recv_map.end()) {
    return false;
  }
 
  uint64_t stval_id=recv_map.at(recv_d->desc_id);
  
  stval_svb_map[stval_id]--;
  assert(stval_svb_map[stval_id]>=0); 
  return true;
}

bool DESCQ::stvalFwdPending(DynamicNode* stval_d) {
  if(stval_svb_map.find(stval_d->desc_id)==stval_svb_map.end()) {
    return false;
  }
  return stval_svb_map[stval_d->desc_id]>0;
}

bool DESCQ::insert(DynamicNode* d) {
  bool canInsert=true;
  //luwa: should consider pushing ld_prod insertion request directly into term load buffer first
  if(d->n->typeInstr==LD_PROD) { //should be entry in comm queue   
    if(supply_count==supply_size) {  
      canInsert=false;
      if(d->issued && d!=term_ld_buffer.front()) { //don't push here if just testing if there are resources
        term_ld_buffer.push_back(d); //we're mostly using this so we don't lose the return from memory, only relevant when handlememoryreturn function calls insert
      }
    }
    else {     
      supply_count++;
    }
  }
  if(d->n->typeInstr==SEND) {     
    if(supply_count==supply_size)  
      canInsert=false;
    else
      supply_count++;  
  }
  if(d->n->typeInstr==RECV) {    
    if(consume_count==consume_size) {     
      canInsert=false;
    }
    else {     
      consume_count++;      
    }
  }
  if(d->n->typeInstr==STADDR) {
    insert_staddr_map(d);
  }
  if(d->n->typeInstr==STVAL) {
    stval_svb_map[d->desc_id]=0;
  }
  
  if(canInsert)
    pq.push(make_pair(d,cycles+latency));  
  
  return canInsert;
}

Simulator::Simulator() {        
  descq = new DESCQ();
  descq->consume_size=cfg.consume_size;
  descq->supply_size=cfg.supply_size;
  descq->term_buffer_size=cfg.term_buffer_size;
    
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
  if(d->n->typeInstr == LD_PROD) {     
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

void Simulator::InsertCaches(vector<Transaction*>& transVec) {
  for (auto it=transVec.begin(); it!=transVec.end(); ++it) {
    Transaction* t=*it;        
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
    delete t;
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
    
    if(cores[0]->cycles % 1000000 == 0 && cores[0]->cycles !=0) {     
      curr_time = Clock::now();
      uint64_t tdiff = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      //uint64_t tdiff_mins = chrono::duration_cast<std::chrono::milliseconds>(curr_time - last_time).count();
      double instr_rate = ((double)(stat.get("total_instructions") - last_instr_count)) / tdiff;
      cout << "Global Simulation Speed: " << instr_rate << " Instructions per ms \n";
      
      uint64_t remaining_instructions = total_instructions - last_instr_count; 
      
      double remaining_time = (double)remaining_instructions/(60000*instr_rate);
      cout << "Remaining Time: " << (int)remaining_time << " mins \n Remaining Instructions: " << remaining_instructions << endl;
      
      last_instr_count = stat.get("total_instructions");
      last_time = curr_time;
    }
    else if(cores[0]->cycles == 0) {
      last_time = Clock::now();
      last_instr_count = 0;
    }          
  }
  
  //print stats for each pythia core
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
  curr_time=Clock::now();
  uint64_t tdiff_mins = chrono::duration_cast<std::chrono::minutes>(curr_time - init_time).count();
  uint64_t tdiff_seconds = chrono::duration_cast<std::chrono::seconds>(curr_time - init_time).count();
  uint64_t tdiff_milliseconds = chrono::duration_cast<std::chrono::milliseconds>(curr_time - init_time).count();
  if(tdiff_mins>5) {
    cout << "Total Runtime: " << tdiff_mins << " mins \n";
  }
  else if(tdiff_seconds>0) {
    cout << "Total Runtime: " << tdiff_seconds << " secs \n";
  }
  else
    cout << "Total Runtime: " << tdiff_milliseconds << " ms \n";

  cout << "Average Global Simulation Speed: " << 1000*total_instructions/tdiff_milliseconds << " Instructions per sec \n";
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

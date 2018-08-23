#include "../common.h"
#include "DRAM.h"
#include "Cache.h"
#include "../executionModel/DynamicNode.h"
using namespace std;
#define UNUSED 0

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());
  if(UNUSED)
    cout << id << clock_cycle;
  queue<DynamicNode*> &q = outstanding_read_map.at(addr);
  while(q.size() > 0) {
    DynamicNode* d = q.front();  
    d->handleMemoryReturn();
    q.pop();
  }
  int64_t evictedAddr = -1;
  if(!ideal)
    c->fc->insert(addr/64, &evictedAddr);
  if(evictedAddr!=-1) {
    assert(evictedAddr >= 0);
    stat.update("cache_evict");
    c->to_evict.push_back(evictedAddr*64);
  }
  if(q.size() == 0)
    outstanding_read_map.erase(addr);
}
void DRAMSimInterface::write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  if(UNUSED)
    cout << id << addr << clock_cycle;
}
void DRAMSimInterface::addTransaction(DynamicNode* d, uint64_t addr, bool isLoad) {
  if(isLoad) 
    mem_load_ports--;
  else
    mem_store_ports--;
  if(d!= NULL) {
    assert(isLoad == true);
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<DynamicNode*>()));
      mem->addTransaction(false, addr);
      stat.update("dram_access");
    }
    outstanding_read_map.at(addr).push(d);  
  }
  else {
    assert(isLoad == false);
    mem->addTransaction(true, addr);
    stat.update("dram_access");
  }
}
bool DRAMSimInterface::willAcceptTransaction(DynamicNode* d, uint64_t addr) {
  return mem->willAcceptTransaction(addr) && !(d->type==LD && mem_load_ports==0) && !(d->type==ST && mem_store_ports==0);
}
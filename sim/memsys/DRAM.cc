#include "DRAM.h"
#include "Cache.h"
using namespace std;
#define UNUSED 0

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());
  if(UNUSED)
    cout << id << clock_cycle;
  queue<Transaction*> &q = outstanding_read_map.at(addr);
  while(q.size() > 0) {
    Transaction* t = q.front();  
    c->TransactionComplete(t);
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
void DRAMSimInterface::addTransaction(Transaction* d, uint64_t addr, bool isLoad) {
  if(isLoad) 
    free_load_ports--;
  else
    free_store_ports--;
  if(d!= NULL) {
    assert(isLoad == true);
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<Transaction*>()));
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
bool DRAMSimInterface::willAcceptTransaction(uint64_t addr, bool isLoad) {
  if((free_load_ports == 0 && isLoad) || (free_store_ports == 0 && !isLoad))
    return false;
  return mem->willAcceptTransaction(addr);
}